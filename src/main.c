#include <errno.h>
#include <stdio.h>

#include "connection_handler.h"
#include "parse.h"

#include <FreeImage.h>

int main(int argc, char **argv) {

  struct arguments args = parse_args(argc, argv);
  struct configuration config = parse_conf(args);

  printf("Config:\n");
  printf("\tPort #%d\n", config.port);
  printf("\tEqualized images dir: %s\n", config.dl_equalizer_path);
  printf("\tColor images dir: %s\n", config.dl_colors_path);
  printf("\tLog file: %s\n\n", config.log_path);

  FreeImage_Initialise(true);

  struct MHD_Daemon *daemon = MHD_start_daemon(
      MHD_USE_INTERNAL_POLLING_THREAD, config.port, &on_client_connect, NULL,
      &connection_handler, &config, MHD_OPTION_NOTIFY_COMPLETED,
      &request_completed, NULL, MHD_OPTION_END);

  if (NULL == daemon) {
    fprintf(stderr, "Failed to start server daemon");
    return ECONNREFUSED;
  }

  char c;
  while ((c = getchar()) != 'q') {
  }

  FreeImage_DeInitialise();

  return 0;
}
