#ifndef APPEARANCE_THEME_IMAGE_H
#define APPEARANCE_THEME_IMAGE_H

#include <cairo.h>

#include "xpm.h"

typedef struct theme_image_source {
    const char *dir;
    const xpm_symbols *symbols;
} theme_image_source;

cairo_surface_t *appearance_theme_image_load(const theme_image_source *source, const char *name);

#endif
