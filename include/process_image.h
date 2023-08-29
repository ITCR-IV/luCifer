#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

#include <FreeImage.h>

enum ImageOperation {
	CLASSIFY_RGB,
	EQUALIZE_HISTOGRAM,
};

enum DominantColor {
	RED, GREEN, BLUE
};

#define RED_COLOR_DIR "rojo"
#define BLUE_COLOR_DIR "blue"
#define GREEN_COLOR_DIR "verde"

enum DominantColor get_dominant_color(FIBITMAP *img);
const char* get_dominant_color_subdir(enum DominantColor color);
void equalize_histogram(FIBITMAP *img);

#endif
