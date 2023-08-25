#include "connection_handler.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "util.h"

enum MHD_Result connection_handler(void *cls, struct MHD_Connection *connection,
                                   const char *url, const char *method,
                                   const char *version, const char *upload_data,
                                   size_t *upload_data_size, void **con_cls) {
  UNUSED(cls);
  UNUSED(url);
  UNUSED(version);
  UNUSED(upload_data);
  UNUSED(upload_data_size);

  printf("New %s request for %s using version %s\n", method, url, version);

  static int aptr;

  if (0 != strcmp(method, "GET"))
    return MHD_NO; /* unexpected method */
  if (&aptr != *con_cls) {
    /* do never respond on first call */
    *con_cls = &aptr;
    return MHD_YES;
  }

  // Reset ptr
  *con_cls = NULL;

  const char *page = "<html><body>Hello, browser!</body></html>";

  struct MHD_Response *response = MHD_create_response_from_buffer(
      strlen(page), (void *)page, MHD_RESPMEM_PERSISTENT);

  int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);

  return ret;
}
