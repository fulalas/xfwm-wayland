#ifndef APPEARANCE_THEME_H
#define APPEARANCE_THEME_H

#include <cairo.h>

#include "theme_image.h"
#include "wm/decor.h"

typedef struct theme theme;

typedef struct theme_source {
    theme_image_source images;
    int pixmap_scale;
} theme_source;

theme *appearance_theme_create(const theme_source *source);
void appearance_theme_destroy(theme *theme);
cairo_surface_t *appearance_theme_pixmap(const theme *theme, wm_pixmap_ref ref);
int appearance_theme_pixmap_scale(const theme *theme);
wm_theme_metrics appearance_theme_metrics(const theme *theme);

#endif
