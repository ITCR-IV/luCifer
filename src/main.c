#include <errno.h>
#include <stdio.h>

#include "connection_handler.h"
#include "parse.h"

int main(int argc, char **argv) {

  struct arguments args = parse(argc, argv);

  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, args.port,
                            &on_client_connect, NULL, &connection_handler,
                            &request_completed, MHD_OPTION_END);
  if (NULL == daemon) {
    fprintf(stderr, "Failed to start server daemon");
    return ECONNREFUSED;
  }

  getchar();

  MHD_stop_daemon(daemon);

  return 0;
}
