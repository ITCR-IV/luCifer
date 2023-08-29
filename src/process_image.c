#include "process_image.h"

enum DominantColor get_dominant_color(FIBITMAP *img) { return RED; }

void equalize_histogram(FIBITMAP *img) {}

const char *get_dominant_color_subdir(enum DominantColor color) {
  switch (color) {
  case RED:
    return RED_COLOR_DIR;
  case GREEN:
    return GREEN_COLOR_DIR;
  case BLUE:
    return BLUE_COLOR_DIR;
    break;
  }
	return "INVALID_COLOR";
}
