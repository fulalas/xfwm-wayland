#ifndef APPEARANCE_CANVAS_H
#define APPEARANCE_CANVAS_H

#include <cairo.h>

#include "wm/geometry.h"

typedef struct canvas {
    cairo_surface_t *surface;
    cairo_t *cr;
} canvas;

typedef struct canvas_image {
    cairo_surface_t *surface;
    wm_rect rect;
    double alpha;
} canvas_image;

canvas appearance_canvas_create(wm_size logical, double scale);
cairo_surface_t *appearance_canvas_finish(canvas *canvas);
void appearance_canvas_rounded_rect(cairo_t *cr, const wm_rect *rect, double radius);
void appearance_canvas_paint_fitted(cairo_t *cr, const canvas_image *image);
wm_size appearance_surface_size(cairo_surface_t *surface);
wm_size appearance_surface_logical_size(cairo_surface_t *surface, double scale);

#endif
