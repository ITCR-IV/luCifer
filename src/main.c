#include <errno.h>
#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "parse.h"

#define UNUSED(x) (void)x

enum MHD_Result answer_to_connection(void *cls,
                                     struct MHD_Connection *connection,
                                     const char *url, const char *method,
                                     const char *version,
                                     const char *upload_data,
                                     size_t *upload_data_size, void **con_cls) {

  UNUSED(cls);
  UNUSED(url);
  UNUSED(method);
  UNUSED(version);
  UNUSED(upload_data);
  UNUSED(upload_data_size);
  UNUSED(con_cls);

  const char *page = "<html><body>Hello, browser!</body></html>";

  struct MHD_Response *response = MHD_create_response_from_buffer(
      strlen(page), (void *)page, MHD_RESPMEM_PERSISTENT);

  int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);

  return ret;
}

int main(int argc, char **argv) {

  struct arguments args = parse(argc, argv);

  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, args.port, NULL,
                            NULL, &answer_to_connection, NULL, MHD_OPTION_END);
  if (NULL == daemon) {
    fprintf(stderr, "Failed to start server daemon");
    return ECONNREFUSED;
  }

  getchar();

  MHD_stop_daemon(daemon);

  return 0;
}
