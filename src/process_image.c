#include "process_image.h"

#include <FreeImage.h>
#include <stdio.h>
#include <stdlib.h>

size_t max_of_two(size_t x, size_t y) { return (x > y) ? x : y; }

size_t is_max_of_three(size_t check, size_t m, size_t n) {
  return check > max_of_two(m, n);
}

enum DominantColor get_dominant_color(FIBITMAP *image) {

  unsigned WIDTH = FreeImage_GetWidth(image);
  unsigned HEIGHT = FreeImage_GetHeight(image);

  RGBQUAD color;

  size_t red_count = 0;
  size_t blue_count = 0;
  size_t green_count = 0;

  for (unsigned i = 0; i < WIDTH; i++) {
    for (unsigned j = 0; j < HEIGHT; j++) {
      if (FreeImage_GetPixelColor(image, i, j, &color)) {
        red_count += color.rgbRed;
        blue_count += color.rgbBlue;
        green_count += color.rgbGreen;
      }
    }
  }

  if ((red_count >= blue_count) && (red_count >= green_count))
    return RED;

  if ((blue_count >= red_count) && (blue_count >= green_count))
    return BLUE;

  if ((green_count >= red_count) && (green_count >= red_count))
    return GREEN;

  // Unreachable
  fprintf(stderr, "Unreachable reached!");
  exit(5);
}

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
