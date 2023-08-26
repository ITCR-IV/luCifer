#include <errno.h>
#include <stdio.h>

#include "connection_handler.h"
#include "parse.h"

int main(int argc, char **argv) {

  struct arguments args = parse_args(argc, argv);
  struct configuration config = parse_conf(args);

  printf("Config:\n");
  printf("\tPort #%d\n", config.port);
  printf("\tEqualized images dir: %s\n", config.dl_equalizer_path);
  printf("\tColor images dir: %s\n", config.dl_colors_path);
  printf("\tLog file: %s\n\n", config.log_path);

  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, args.port,
                            &on_client_connect, &config, &connection_handler,
                            &request_completed, MHD_OPTION_END);
  if (NULL == daemon) {
    fprintf(stderr, "Failed to start server daemon");
    return ECONNREFUSED;
  }

  while (1) {
  }

  return 0;
}
