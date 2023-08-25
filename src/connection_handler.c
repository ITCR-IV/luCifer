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

#include "util.h"

#define POSTBUFFERSIZE 65536
#define MAXCLIENTS 2

enum ConnectionType {
  GET,
  POST,
};

static unsigned int nr_of_uploading_clients = 0;

const char *busypage =
    "<html><body>This server is busy, please try again later.</body></html>";
const char *askpage = "<html><body>\n\
                       Upload a file, please!<br>\n\
                       There are %u clients uploading at the moment.<br>\n\
                       <form action=\"/filepost\" method=\"post\" \
                         enctype=\"multipart/form-data\">\n\
                       <input name=\"file\" type=\"file\">\n\
                       <input type=\"submit\" value=\" Send \"></form>\n\
                       </body></html>";
const char *completepage =
    "<html><body>The upload has been completed.</body></html>";
const char *servererrorpage =
    "<html><body>An internal server error has occurred.</body></html>";
const char *fileexistspage =
    "<html><body>This file already exists.</body></html>";
const char *errorpage =
    "<html><body>This doesn't seem to be right.</body></html>";

enum MHD_Result print_out_key(void *cls, enum MHD_ValueKind kind,
                              const char *key, const char *value) {

  UNUSED(cls);
  UNUSED(kind);

  printf("%s: %s\n", key, value);
  return MHD_YES;
}

enum MHD_Result send_page(struct MHD_Connection *connection, const char *page,
                          unsigned int status_code) {
  struct MHD_Response *response = MHD_create_response_from_buffer(
      strlen(page), (void *)page, MHD_RESPMEM_MUST_COPY);

  if (!response)
    return MHD_NO;

  enum MHD_Result ret = MHD_queue_response(connection, status_code, response);
  MHD_destroy_response(response);

  return ret;
}

enum MHD_Result on_client_connect(void *cls, const struct sockaddr *addr,
                                  socklen_t addrlen) {
  UNUSED(cls);
  UNUSED(addrlen);

  struct in_addr address = ((struct sockaddr_in *)addr)->sin_addr;

  printf("\nIP: %s\n", inet_ntoa(address));

  return MHD_YES;
}

struct connection_info_struct {
  enum ConnectionType connectiontype;
  FILE *fp;
  struct MHD_PostProcessor *postprocessor;
  const char *answerstring;
  unsigned int answercode;
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

  // Por default si algo sale mal se devuelve este error
  con_info->answerstring = servererrorpage;
  con_info->answercode = MHD_HTTP_INTERNAL_SERVER_ERROR;

  // Le decimos al cliente que ponga los datos bajo la etiqueta "file"
  if (0 != strcmp(key, "file")) {
    return MHD_NO;
  }

  FILE *fp;

  if (!con_info->fp) {
    if (NULL != (fp = fopen(filename, "rb"))) {
      fclose(fp);
      con_info->answerstring = fileexistspage;
      con_info->answercode = MHD_HTTP_FORBIDDEN;
      return MHD_NO;
    }

    con_info->fp = fopen(filename, "ab");
    if (!con_info->fp) {
      return MHD_NO;
    }
  }

  if (size > 0) {
    if (!fwrite(data, sizeof(char), size, con_info->fp)) {
      return MHD_NO;
    }
  }

  con_info->answerstring = completepage;
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
      nr_of_uploading_clients--;
    }

    if (con_info->fp)
      fclose(con_info->fp);
  }

  free(con_info);
  *con_cls = NULL;
}

enum MHD_Result connection_handler(void *cls, struct MHD_Connection *connection,
                                   const char *url, const char *method,
                                   const char *version, const char *upload_data,
                                   size_t *upload_data_size, void **con_cls) {

  UNUSED(cls);

  // Primera conexión
  if (*con_cls == NULL) {
    printf("New %s request for %s using version %s\n", method, url, version);
    MHD_get_connection_values(connection, MHD_HEADER_KIND | MHD_POSTDATA_KIND,
                              &print_out_key, NULL);

    if (nr_of_uploading_clients >= MAXCLIENTS)
      return send_page(connection, busypage, MHD_HTTP_SERVICE_UNAVAILABLE);

    struct connection_info_struct *con_info =
        malloc(sizeof(struct connection_info_struct));

    if (NULL == con_info) {
      return MHD_NO;
    }

    con_info->answerstring = NULL;
    con_info->fp = 0;

    if (0 == strcmp(method, "POST")) {
      con_info->postprocessor = MHD_create_post_processor(
          connection, POSTBUFFERSIZE, iterate_post, (void *)con_info);

      if (NULL == con_info->postprocessor) {
        free(con_info);
        return MHD_NO;
      }

      nr_of_uploading_clients++;

      con_info->connectiontype = POST;
      con_info->answercode = MHD_HTTP_OK;
      con_info->answerstring = completepage;
    } else
      con_info->connectiontype = GET;

    *con_cls = (void *)con_info;
    return MHD_YES;
  }

  if (strcmp(method, "GET") == 0) {
    char buffer[1024];

    sprintf(buffer, askpage, nr_of_uploading_clients);
    return send_page(connection, buffer, MHD_HTTP_OK);
  }

  if (strcmp(method, "POST") == 0) {

    struct connection_info_struct *con_info = *con_cls;

    if (*upload_data_size != 0) {
      MHD_post_process(con_info->postprocessor, upload_data, *upload_data_size);
      *upload_data_size = 0;

      return MHD_YES;
    } else if (NULL != con_info->answerstring)
      return send_page(connection, con_info->answerstring,
                       con_info->answercode);
  }

  return send_page(connection, errorpage, MHD_HTTP_BAD_REQUEST);
}
