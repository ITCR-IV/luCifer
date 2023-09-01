#include "process_image.h"

#include <FreeImage.h>
#include <math.h>
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

// This function calculates the cumulative distribution function (cdf) of a
// histogram, normalizes it to 0-255, and casts it into BYTE
//
// in:  histogram, to calculate cdf from
// out: cdf_LUT, buffer is expected to be 256 items long
void cdf_lut(DWORD histogram[256], BYTE *cdf_LUT) {
  // Get cumulative density function
  size_t cdf[256] = {0};

  // Print histograma
  // printf("Histograma: [ %d", histogram[0]);

  // for (int i = 1; i < 255; i++) {
  //   printf(", %08X", histogram[i]);
  // }
  // printf(" ]\n");

  cdf[0] = histogram[0];
  for (size_t i = 1; i < 256; i++) {
    cdf[i] = cdf[i - 1] + histogram[i];
  }

  // Normalize it to 255
  float normalized_cdf[256];
  float divisor = (float)cdf[255] / 255.0;

  for (size_t i = 0; i < 256; i++) {
    normalized_cdf[i] = (float)cdf[i] / divisor;
  }

  // Round
  for (size_t i = 0; i < 256; i++) {
    unsigned int whole = (unsigned int)round(normalized_cdf[i]);
    cdf_LUT[i] = whole;
  }
}

int equalize_histogram(FIBITMAP **img) {
  DWORD histogram[256] = {0};
  BYTE cdf_LUT[256];

  if (!FreeImage_GetHistogram(*img, histogram, FICC_RED)) {
    return 1;
  }
  cdf_lut(histogram, cdf_LUT);
  if (!FreeImage_AdjustCurve(*img, cdf_LUT, FICC_RED)) {
    return 1;
  }

  if (!FreeImage_GetHistogram(*img, histogram, FICC_BLUE)) {
    return 1;
  }
  cdf_lut(histogram, cdf_LUT);
  if (!FreeImage_AdjustCurve(*img, cdf_LUT, FICC_BLUE)) {
    return 1;
  }

  if (!FreeImage_GetHistogram(*img, histogram, FICC_GREEN)) {
    return 1;
  }
  cdf_lut(histogram, cdf_LUT);
  if (!FreeImage_AdjustCurve(*img, cdf_LUT, FICC_GREEN)) {
    return 1;
  }

  return 0;
}

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
