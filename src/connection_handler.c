#include "connection_handler.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#include "parse.h"
#include "process_image.h"
#include "util.h"

#define POSTBUFFERSIZE 65536
#define MAXCLIENTS 2

enum ConnectionType {
  GET,
  POST,
};

const char *askpage = "<html><body>\n\
                       Upload a file, please!<br>\n\
                       <form action=\"/equalize\" method=\"post\" \
                         enctype=\"multipart/form-data\">\n\
                       <input name=\"file\" type=\"file\">\n\
                       <input type=\"submit\" value=\" Send \"></form>\n\
                       </body></html>";
const char *complete_response = "The upload has been completed.";
const char *server_error_response =
    "500 An internal server error has occurred.";
const char *file_exists_response = "This file already exists.";
const char *bad_request_response = "This doesn't seem to be right.";
const char *unrecognized_image_response = "Unrecognized image format.";
const char *not_found_response = "404 Page Not Found.";

enum MHD_Result send_response(struct MHD_Connection *connection,
                              const char *page, unsigned int status_code) {
  struct MHD_Response *response = MHD_create_response_from_buffer(
      strlen(page), (void *)page, MHD_RESPMEM_MUST_COPY);

  if (!response) {
    return MHD_NO;
  }

  enum MHD_Result ret = MHD_queue_response(connection, status_code, response);
  MHD_destroy_response(response);

  return ret;
}

int log_info(struct configuration *config, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  FILE *fp;
  if (NULL != (fp = fopen(config->log_path, "a"))) {

    time_t t;
    time(&t);
    struct tm *timeinfo = localtime(&t);

    char date_time[64];
    strftime(date_time, 64, "%Y %m %d %T", timeinfo);

    fprintf(fp, "%s: ", date_time);
    vfprintf(fp, fmt, args);
    fprintf(fp, "\n");

    fclose(fp);
  }

  va_start(args, fmt);
  int ret = vprintf(fmt, args);
  printf("\n");
  va_end(args);
  return ret;
}

enum MHD_Result on_client_connect(void *cls, const struct sockaddr *addr,
                                  socklen_t addrlen) {
  UNUSED(addrlen);

  struct configuration *config = cls;
  struct in_addr address = ((struct sockaddr_in *)addr)->sin_addr;

  log_info(config, "New client connection with IP: %s", inet_ntoa(address));

  return MHD_YES;
}

struct connection_info_struct {
  enum ConnectionType connectiontype;
  FILE *fp;
  const char *tmp_filename;
  struct MHD_PostProcessor *postprocessor;
  const char *answerstring;
  unsigned int answercode;
  const struct configuration *config;
  enum ImageOperation operation;
};

static enum MHD_Result iterate_post(void *coninfo_cls, enum MHD_ValueKind kind,
                                    const char *key, const char *filename,
                                    const char *content_type,
                                    const char *transfer_encoding,
                                    const char *data, uint64_t off,
                                    size_t size) {

  UNUSED(kind);
  UNUSED(content_type);
  UNUSED(transfer_encoding);
  UNUSED(off);

  struct connection_info_struct *con_info = coninfo_cls;

  // Default return if anything goes unexpectedly horribly wrong
  con_info->answerstring = server_error_response;
  con_info->answercode = MHD_HTTP_INTERNAL_SERVER_ERROR;

  // The client should put the data under the "file" tag
  if (0 != strcmp(key, "file")) {
    return MHD_NO;
  }

  FILE *fp;

  // On first iteration
  if (!con_info->fp) {
    if (filename == NULL) {
      con_info->answerstring = "Bad Request: filename not provided";
      con_info->answercode = MHD_HTTP_BAD_REQUEST;
      return MHD_NO;
    }

    const char *tmp_filename = malloc(strlen(filename) + 2);
    sprintf((char *)tmp_filename, ".%s", filename);

    if (NULL != (fp = fopen(tmp_filename, "rb"))) {
      fclose(fp);
      con_info->answerstring = file_exists_response;
      con_info->answercode = MHD_HTTP_FORBIDDEN;
      return MHD_NO;
    }

    con_info->fp = fopen(tmp_filename, "ab");
    con_info->tmp_filename = tmp_filename;

    if (!con_info->fp) {
      con_info->answerstring = server_error_response;
      con_info->answercode = MHD_HTTP_INTERNAL_SERVER_ERROR;
      return MHD_NO;
    }
  }

  if (size > 0) {
    if (!fwrite(data, sizeof(char), size, con_info->fp)) {
      con_info->answerstring = server_error_response;
      con_info->answercode = MHD_HTTP_INTERNAL_SERVER_ERROR;
      return MHD_NO;
    }
  }

  con_info->answerstring = complete_response;
  con_info->answercode = MHD_HTTP_OK;

  return MHD_YES;
}

void request_completed(void *cls, struct MHD_Connection *connection,
                       void **con_cls, enum MHD_RequestTerminationCode toe) {
  UNUSED(cls);
  UNUSED(connection);
  UNUSED(toe);

  struct connection_info_struct *con_info = *con_cls;

  if (NULL == con_info)
    return;

  if (con_info->connectiontype == POST) {
    if (NULL != con_info->postprocessor) {
      MHD_destroy_post_processor(con_info->postprocessor);
    }

    if (con_info->fp)
      fclose(con_info->fp);
  }

  if (con_info->tmp_filename != NULL) {
    FILE *fp;
    if (NULL != (fp = fopen(con_info->tmp_filename, "rb"))) {
      fclose(fp);
      remove(con_info->tmp_filename);
    }
    free((void *)con_info->tmp_filename);
  }

  free(con_info);
  *con_cls = NULL;
}

void final_processing(struct connection_info_struct *con_info) {

  if (con_info->tmp_filename == NULL) {
    con_info->answerstring =
        "Bad Request: No file was provided (under 'file' tag)";
    con_info->answercode = MHD_HTTP_BAD_REQUEST;
    return;
  }

  // Get image format
  FREE_IMAGE_FORMAT image_format =
      FreeImage_GetFileType(con_info->tmp_filename, 0);
  if (image_format == FIF_UNKNOWN) {
    // try to guess the file format from the file extension
    image_format = FreeImage_GetFIFFromFilename(con_info->tmp_filename);
  }
  FIBITMAP *dib;

  // Load image
  if ((image_format != FIF_UNKNOWN) &&
      FreeImage_FIFSupportsReading(image_format)) {

    fclose(con_info->fp);
    con_info->fp = NULL;

    dib = FreeImage_Load(image_format, con_info->tmp_filename, 0);
  } else {
    con_info->answerstring = unrecognized_image_response;
    con_info->answercode = MHD_HTTP_BAD_REQUEST;
    return;
  }

  if (dib == NULL) {
    con_info->answerstring = server_error_response;
    con_info->answercode = MHD_HTTP_INTERNAL_SERVER_ERROR;
    return;
  }

  remove(con_info->tmp_filename);

  // TODO: post-process (check operation, process image, save final file,
  // delete tmp file)
  const char *full_pathname;

  switch (con_info->operation) {
  case CLASSIFY_RGB:;
    enum DominantColor color = get_dominant_color(dib);
    full_pathname = malloc(strlen(con_info->config->dl_colors_path) +
                           strlen(&(con_info->tmp_filename[1])) +
                           strlen(get_dominant_color_subdir(color)) + 3);
    sprintf((char *)full_pathname, "%s/%s/%s", con_info->config->dl_colors_path,
            get_dominant_color_subdir(color), &(con_info->tmp_filename[1]));

    break;
  case EQUALIZE_HISTOGRAM:
    equalize_histogram(dib);

    full_pathname = malloc(strlen(con_info->config->dl_equalizer_path) +
                           strlen(&(con_info->tmp_filename[1])) + 2);
    sprintf((char *)full_pathname, "%s/%s", con_info->config->dl_equalizer_path,
            &(con_info->tmp_filename[1]));

    break;
  default:
    con_info->answerstring = server_error_response;
    con_info->answercode = MHD_HTTP_INTERNAL_SERVER_ERROR;
    FreeImage_Unload(dib);
    return;
  }

  FILE *fp;
  if (NULL != (fp = fopen(full_pathname, "rb"))) {
    fclose(fp);
    con_info->answerstring = file_exists_response;
    con_info->answercode = MHD_HTTP_FORBIDDEN;
    return;
  }
  FreeImage_Save(image_format, dib, full_pathname, 0);

  free((void *)full_pathname);
  FreeImage_Unload(dib);

  con_info->answerstring = complete_response;
  con_info->answercode = MHD_HTTP_OK;
}

enum MHD_Result connection_handler(void *cls, struct MHD_Connection *connection,
                                   const char *url, const char *method,
                                   const char *version, const char *upload_data,
                                   size_t *upload_data_size, void **con_cls) {

  // First connection inside this if sets stuff up
  if (*con_cls == NULL) {
    log_info(cls, "%s request for %s using version %s", method, url, version);

    struct connection_info_struct *con_info =
        malloc(sizeof(struct connection_info_struct));

    if (NULL == con_info) {
      return MHD_NO;
    }

    con_info->config = cls;
    con_info->answerstring = NULL;
    con_info->tmp_filename = NULL;
    con_info->fp = 0;

    if (0 == strcmp(method, "POST")) {
      if (0 == strcmp(url, "/equalize")) {
        con_info->operation = EQUALIZE_HISTOGRAM;

      } else if (0 == strcmp(url, "/classify/rgb")) {
        con_info->operation = CLASSIFY_RGB;
      }

      con_info->postprocessor = MHD_create_post_processor(
          connection, POSTBUFFERSIZE, iterate_post, (void *)con_info);

      if (NULL == con_info->postprocessor) {
        free(con_info);
        return MHD_NO;
      }

      con_info->connectiontype = POST;
      con_info->answercode = MHD_HTTP_OK;
      con_info->answerstring = complete_response;
    } else
      con_info->connectiontype = GET;

    *con_cls = (void *)con_info;
    return MHD_YES;
  }

  if (strcmp(method, "GET") == 0) {
    return send_response(connection, askpage, MHD_HTTP_OK);
  }

  if (strcmp(method, "POST") == 0) {

    if (!(0 == strcmp(url, "/equalize")) &&
        !(0 == strcmp(url, "/classify/rgb"))) {
      if (*upload_data_size != 0) {
        *upload_data_size = 0;
        return MHD_YES;
      }
      return send_response(connection, not_found_response, MHD_HTTP_NOT_FOUND);
    }

    struct connection_info_struct *con_info = *con_cls;

    if (*upload_data_size != 0) {
      MHD_post_process(con_info->postprocessor, upload_data, *upload_data_size);
      *upload_data_size = 0;

      return MHD_YES;
    } else if (con_info->answercode != MHD_HTTP_OK) {
      return send_response(connection, con_info->answerstring,
                           con_info->answercode);
    } else {
      final_processing(con_info);
      if (NULL != con_info->answerstring) {

        // Here do final processing before completing response
        return send_response(connection, con_info->answerstring,
                             con_info->answercode);
      }
    }
  }

  return send_response(connection, bad_request_response, MHD_HTTP_BAD_REQUEST);
}
