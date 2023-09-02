#include "parse.h"

#include <argp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "process_image.h"

#define DEFAULT_PORT 1717
#define DEFAULT_CONFIG "/etc/ImageServer/config.conf"
#define DEFAULT_COLORS_DIR "colors"
#define DEFAULT_EQU_DIR "equalized"
#define DEFAULT_LOG_FILE "/var/log/image-server"

static char doc[] = "luCifer -- server that accepts and processes images";

static char args_doc[] = "";

static struct argp_option options[] = {
    {"port", 'p', "PORT", 0, "Specify a different port (default: 1717)", 0},
    {"conf", -1, "CONF_FILE", 0,
     "Specify a configuration file (default: " DEFAULT_CONFIG ")", 0},
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
      argp_error(state, "Failed to parse port number (must be 1-65535)");
    }
    if (port <= 0 || port > 65535) {
      argp_error(state, "Port number must be 1-65535");
    }

    arguments->port = (uint16_t)port;
    arguments->specified_port = true;
    break;

  case -1:
    if (arguments->conf_file != NULL) {
      fclose(arguments->conf_file);
    }

    arguments->conf_file = fopen(arg, "r");

    if (arguments->conf_file == NULL) {
      argp_error(state, "Failed to open configuration file '%s'", arg);
    }
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
struct arguments parse_args(int argc, char **argv) {
  struct arguments arguments;

  /* Default values. */
  arguments.port = DEFAULT_PORT;
  arguments.specified_port = false;
  // Might be NULL if it doesn't exist which is ok (will use default values)
  arguments.conf_file = fopen(DEFAULT_CONFIG, "r");

  error_t ret = argp_parse(&argp, argc, argv, 0, 0, &arguments);

  if (ret != 0) {
    fprintf(stderr, "Error parsing arguments: %s\n", strerror(ret));
    exit(ret);
  }

  return arguments;
}

#define PORT_KEY "puerto"
#define COLORS_PATH_KEY "colores"
#define EQU_PATH_KEY "ecualizado"
#define LOG_PATH_KEY "log"

// Biggest is "ecualizado" which is 12 bytes
#define KEY_BUF_SIZE 16
#define LINE_BUF_SIZE 1024
#define PATH_BUF_SIZE 1024

char abspath[PATH_BUF_SIZE];

// Might exit with error code
void create_dir(const char *dirname) {
  realpath(dirname, abspath);

  struct stat st = {0};
  if (stat(abspath, &st) == -1) {
    if (mkdir(abspath, 0777) != 0) {
      fprintf(stderr, "Error creating/reading dir '%s': %s\n", abspath,
              strerror(errno));
      exit(errno);
    }
  }
  stat(abspath, &st);
  if (!S_ISDIR(st.st_mode)) {
    fprintf(stderr, "Error: '%s' file already exists and is not a directory.\n",
            abspath);
    exit(4);
  }
}

// conf_file: FILE* to configuration file
// line: buffer to store line
// out_key: buffer to store key
// line_num: number of line (gets incremented at the start)
// c: char that gets passed to getc, at the end will contain last char read to
// compare with EOF returns: value assigned to key may exit with an error code
//
// Returns: Value read for the out_key, if return is NULL means line is empty
// and should just be skipped
const char *parse_line(FILE *conf_file, char *line, char *out_key,
                       size_t *line_num, char *c) {

  (*line_num)++;
  // First get whole line
  size_t line_i = 0;
  while (((*c = fgetc(conf_file)) != EOF) && *c != '\n' && *c != '#') {
    line[line_i] = *c;
    line_i++;

    if (line_i > LINE_BUF_SIZE) {
      line[LINE_BUF_SIZE] = 0;
      fprintf(stderr,
              "Error on line %zu: Error parsing config file line (too "
              "long): %s\n",
              *line_num, line);
      exit(1);
    }
  }

  line[line_i] = '\0';

  // Skip comments
  if (*c == '#') {
    do {
      *c = fgetc(conf_file);
    } while (*c != EOF && *c != '\n');
  }

  line_i = 0;
  // First skip whitespaces
  while (isspace(line[line_i])) {
    line_i++;
  }

  // If line is just empty, skip
  if (line[line_i] == '\0') {
    return NULL;
  }

  // Get key
  size_t key_i = 0;
  while ((line[line_i] != ':') && (line[line_i] != '\0') &&
         !isspace(line[line_i])) {

    out_key[key_i] = line[line_i];
    line_i++;
    key_i++;

    if (key_i > KEY_BUF_SIZE) {
      out_key[KEY_BUF_SIZE] = 0;
      fprintf(stderr,
              "Error on line %zu: Error parsing key name (too long): %s\n",
              *line_num, out_key);
      exit(1);
    }
  }

  out_key[key_i] = '\0';

  // Skip whitespaces
  if (isspace(line[line_i])) {
    while (isspace(line[line_i])) {
      line_i++;
    }
  }

  // Verify ':'
  if (line[line_i] != ':') {
    fprintf(stderr,
            "Error on line %zu: Syntax error, expected ':' after '%s'\n",
            *line_num, out_key);
    exit(2);
  } else {
    line_i++;
  }

  // Skip whitespaces
  if (isspace(line[line_i])) {
    while (isspace(line[line_i])) {
      line_i++;
    }
  }

  // Trim whitespaces at end
  const char *value = &line[line_i];

  while (line[line_i] != '\0') {
    line_i++;
  }

  while (isspace(line[line_i - 1])) {
    line[line_i - 1] = '\0';
    line_i--;
  }

  return value;
}

// Esta función cierra el FILE de args.conf_file
struct configuration parse_conf(struct arguments args) {
  struct configuration config;

  /* Default values */
  config.port = DEFAULT_PORT;
  config.dl_colors_path = DEFAULT_COLORS_DIR;
  config.dl_equalizer_path = DEFAULT_EQU_DIR;
  config.log_path = DEFAULT_LOG_FILE;

  /* File parsing */
  if (args.conf_file != NULL) {

    char line[LINE_BUF_SIZE + 1];
    char key[KEY_BUF_SIZE + 1];
    char c;
    size_t line_num = 0;

    do {
      const char *value = parse_line(args.conf_file, line, key, &line_num, &c);

      // Empty line
      if (value == NULL) {
        continue;
      }

      // Parse type of key
      if (strcmp(key, PORT_KEY) == 0) {
        char *endptr;
        long int port = strtoul(value, &endptr, 10);
        if (value == endptr) {
          fprintf(stderr,
                  "Error on line %zu: Failed to parse port number in config "
                  "file (must be 1-65535)",
                  line_num);
          exit(3);
        }
        if (port <= 0 || port > 65535) {
          fprintf(
              stderr,
              "Error on line %zu: Port number in config file must be 1-65535",
              line_num);
          exit(3);
        }

        config.port = (uint16_t)port;

      } else if (strcmp(key, COLORS_PATH_KEY) == 0) {
        config.dl_colors_path = malloc(strlen(value) + 1);
        strcpy((char *)config.dl_colors_path, value);

      } else if (strcmp(key, EQU_PATH_KEY) == 0) {
        config.dl_equalizer_path = malloc(strlen(value) + 1);
        strcpy((char *)config.dl_equalizer_path, value);

      } else if (strcmp(key, LOG_PATH_KEY) == 0) {
        config.log_path = malloc(strlen(value) + 1);
        strcpy((char *)config.log_path, value);

      } else {
        fprintf(stderr, "Error on line %zu: Key not recognized '%s'\n",
                line_num, key);
        exit(2);
      }
    } while (c != EOF);

    fclose(args.conf_file);
  }

  /* Overwrite port if specified as CLI argument */
  if (args.specified_port) {
    config.port = args.port;
  }

  /* Check if log file can be written to */
  FILE *fp;
  if (NULL != (fp = fopen(config.log_path, "a"))) {
    fclose(fp);
  } else {
    printf("WARNING: Can't open log file %s for writing\n", config.log_path);
  }

  /* Create or check for colors dirs */
  create_dir(config.dl_colors_path);
  char *green_dir =
      malloc(strlen(config.dl_colors_path) + strlen(GREEN_COLOR_DIR) + 1);
  char *red_dir =
      malloc(strlen(config.dl_colors_path) + strlen(RED_COLOR_DIR) + 1);
  char *blue_dir =
      malloc(strlen(config.dl_colors_path) + strlen(BLUE_COLOR_DIR) + 1);

  sprintf(green_dir, "%s/%s", config.dl_colors_path, GREEN_COLOR_DIR);
  sprintf(red_dir, "%s/%s", config.dl_colors_path, RED_COLOR_DIR);
  sprintf(blue_dir, "%s/%s", config.dl_colors_path, BLUE_COLOR_DIR);

  create_dir(green_dir);
  create_dir(red_dir);
  create_dir(blue_dir);

  free(green_dir);
  free(red_dir);
  free(blue_dir);

  /* Create or check for equalizer dir */
  create_dir(config.dl_equalizer_path);

  return config;
}
