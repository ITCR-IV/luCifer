#include <FreeImage.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>

#include "connection_handler.h"
#include "parse.h"
#include "util.h"

struct MHD_Daemon *mhd_daemon;

void SIGINT_handler(int signal) {
	UNUSED(signal);
  MHD_stop_daemon(mhd_daemon);
	printf("\n");
}

int main(int argc, char **argv) {

  struct arguments args = parse_args(argc, argv);
  struct configuration config = parse_conf(args);

  printf("Config:\n");
  printf("\tPort #%d\n", config.port);
  printf("\tEqualized images dir: %s\n", config.dl_equalizer_path);
  printf("\tColor images dir: %s\n", config.dl_colors_path);
  printf("\tLog file: %s\n\n", config.log_path);

  mhd_daemon = MHD_start_daemon(
      MHD_USE_INTERNAL_POLLING_THREAD, config.port, &on_client_connect, &config,
      &connection_handler, &config, MHD_OPTION_NOTIFY_COMPLETED,
      &request_completed, NULL, MHD_OPTION_END);

  if (NULL == mhd_daemon) {
    fprintf(stderr, "Failed to start server daemon");
    return ECONNREFUSED;
  }

  // Register SIGINT (Ctrl-C) handler
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = SIGINT_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);

  pause();

  return 0;
}
