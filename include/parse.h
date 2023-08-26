#ifndef PARSE_H
#define PARSE_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

struct arguments {
	bool specified_port;
  uint16_t port;
	FILE *conf_file;
};

struct configuration {
	uint16_t port;
	const char *dl_colors_path;
	const char *dl_equalizer_path;
	const char *log_path;
};

struct arguments parse_args(int argc, char **argv);
struct configuration parse_conf(struct arguments args);

#endif
