#ifndef APPEARANCE_THEME_IMAGE_H
#define APPEARANCE_THEME_IMAGE_H

#include <stdbool.h>

#include <cairo.h>

#include "xpm.h"

typedef struct theme_image_source {
    const char *dir;
    const xpm_symbols *symbols;
    double scale;
} theme_image_source;

bool appearance_theme_images_scalable(const char *dir);
cairo_surface_t *appearance_theme_image_load(const theme_image_source *source, const char *name);

#endif
