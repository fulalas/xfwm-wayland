#ifndef COMPOSITOR_X11_PIXMAP_ICON_H
#define COMPOSITOR_X11_PIXMAP_ICON_H

#include <cairo.h>
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

typedef struct x11_bitmap {
    const uint8_t *data;
    uint32_t stride;
    bool lsb_first;
} x11_bitmap;

typedef struct x11_icon_pixels {
    int width;
    int height;
    uint8_t depth;
    const uint32_t *colors;
    uint32_t color_stride;
    x11_bitmap bits;
    const x11_bitmap *mask;
} x11_icon_pixels;

cairo_surface_t *x11_icon_from_pixels(const x11_icon_pixels *pixels);
cairo_surface_t *x11_pixmap_icon(xcb_connection_t *connection, xcb_pixmap_t pixmap,
                                 xcb_pixmap_t mask);

#endif
