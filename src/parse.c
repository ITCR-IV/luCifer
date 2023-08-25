#include "parse.h"

#include <argp.h>
#include <stdlib.h>
#include <string.h>

static char doc[] = "luCifer -- server that accepts and processes images";

static char args_doc[] = "";

static struct argp_option options[] = {
    {"port", 'p', "PORT", 0, "Specify a different port (default: 8888)", 0},
    {0},
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key) {
  case 'p':;
    char *endptr;

    long int port = strtoul(arg, &endptr, 10);
    if (arg == endptr) {
      argp_error(state, "Failure to parse port number (must be 1-65535)");
    }
    if (port <= 0 || port > 65535) {
      argp_error(state, "Port number must be 1-65535");
    }

    arguments->port = (uint16_t)port;
    break;

  case ARGP_KEY_ARG:
    argp_usage(state);
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

/* Our argp parser. */
static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

// Esta función puede llamar exit()
struct arguments parse(int argc, char **argv) {
  struct arguments arguments;

  /* Default values. */
  arguments.port = 8888;

  error_t ret = argp_parse(&argp, argc, argv, 0, 0, &arguments);

  if (ret != 0) {
    printf("Error parsing arguments: %s\n", strerror(ret));
    exit(ret);
  }

  return arguments;
}
