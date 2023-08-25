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

#define POSTBUFFERSIZE 1024

enum ConnectionType {
  GET,
  POST,
};

static const char *askpage = "<html><body>\
                       What's your name, Sir?<br>\
                       <form action=\"/namepost\" method=\"post\">\
                       <input name=\"name\" type=\"text\"\
                       <input type=\"submit\" ></form>\
                       </body></html>";
static const char *greetingpage =
    "<html><body><h1>Welcome, %s!</center></h1></body></html>";
static const char *errorpage =
    "<html><body>This doesn't seem to be right.</body></html>";

enum MHD_Result print_out_key(void *cls, enum MHD_ValueKind kind,
                              const char *key, const char *value) {

  UNUSED(cls);
  UNUSED(kind);

  printf("%s: %s\n", key, value);
  return MHD_YES;
}

enum MHD_Result send_page(struct MHD_Connection *connection,
                          unsigned int status_code, const char *page) {
  struct MHD_Response *response = MHD_create_response_from_buffer(
      strlen(page), (void *)page, MHD_RESPMEM_PERSISTENT);

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
  char *answerstring;
  struct MHD_PostProcessor *postprocessor;
};

static enum MHD_Result iterate_post(void *coninfo_cls, enum MHD_ValueKind kind,
                                    const char *key, const char *filename,
                                    const char *content_type,
                                    const char *transfer_encoding,
                                    const char *data, uint64_t off,
                                    size_t size) {

  UNUSED(kind);
  UNUSED(filename);
  UNUSED(content_type);
  UNUSED(transfer_encoding);
  UNUSED(off);

  struct connection_info_struct *con_info = coninfo_cls;

  if (0 == strcmp(key, "name")) {
    // Nombre menor a 100 caracteres
    if ((size > 0) && (size <= 100)) {
      size_t answer_size = strlen(greetingpage) + size + 1;
      char *answerstring = malloc(answer_size);
      if (!answerstring)
        return MHD_NO;

      snprintf(answerstring, answer_size, greetingpage, data);
      con_info->answerstring = answerstring;
    } else
      con_info->answerstring = NULL;

    return MHD_NO;
  }

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
    MHD_destroy_post_processor(con_info->postprocessor);
    if (con_info->answerstring)
      free(con_info->answerstring);
  }

  free(con_info);
  *con_cls = NULL;
}

enum MHD_Result connection_handler(void *cls, struct MHD_Connection *connection,
                                   const char *url, const char *method,
                                   const char *version, const char *upload_data,
                                   size_t *upload_data_size, void **con_cls) {

  UNUSED(cls);

  if (*con_cls == NULL) {
    printf("New %s request for %s using version %s\n", method, url, version);
    MHD_get_connection_values(connection, MHD_HEADER_KIND | MHD_POSTDATA_KIND,
                              &print_out_key, NULL);

    struct connection_info_struct *con_info =
        malloc(sizeof(struct connection_info_struct));

    if (NULL == con_info)
      return MHD_NO;

    con_info->answerstring = NULL;

    if (0 == strcmp(method, "POST")) {
      con_info->postprocessor = MHD_create_post_processor(
          connection, POSTBUFFERSIZE, iterate_post, (void *)con_info);

      if (NULL == con_info->postprocessor) {
        free(con_info);
        return MHD_NO;
      }
      con_info->connectiontype = POST;
    } else
      con_info->connectiontype = GET;

    *con_cls = (void *)con_info;
    return MHD_YES;
  }

  if (strcmp(method, "GET") == 0) {
    return send_page(connection, MHD_HTTP_OK, askpage);
  } else if (strcmp(method, "POST") == 0) {

    struct connection_info_struct *con_info = *con_cls;

    if (*upload_data_size != 0) {
      MHD_post_process(con_info->postprocessor, upload_data, *upload_data_size);
      *upload_data_size = 0;

      return MHD_YES;
    } else if (NULL != con_info->answerstring)
      return send_page(connection, MHD_HTTP_OK, con_info->answerstring);

  } else {
    return send_page(connection, MHD_HTTP_BAD_REQUEST, errorpage);
  }

  // Reset ptr
  *con_cls = NULL;

  return MHD_YES;
}
