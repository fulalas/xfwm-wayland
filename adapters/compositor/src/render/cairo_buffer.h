#ifndef COMPOSITOR_CAIRO_BUFFER_H
#define COMPOSITOR_CAIRO_BUFFER_H

#include <cairo.h>
#include <stdbool.h>
#include <stdint.h>

#include "wm/geometry.h"

struct wlr_buffer;
struct wlr_scene_buffer;

struct wlr_buffer *cairo_buffer_create_solid(uint32_t argb);
bool cairo_buffer_attach_sized(struct wlr_scene_buffer *node, cairo_surface_t *surface,
                               wm_size size);

typedef struct pixel_rows {
    const void *data;
    int width;
    int height;
    int stride;
} pixel_rows;

struct wlr_buffer *cairo_buffer_create(cairo_surface_t *surface);
cairo_surface_t *cairo_buffer_copy_pixels(const pixel_rows *rows);

#endif
