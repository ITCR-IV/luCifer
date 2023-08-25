#include <stdint.h>

struct arguments {
  uint16_t port;
};

struct arguments parse(int argc, char **argv);
