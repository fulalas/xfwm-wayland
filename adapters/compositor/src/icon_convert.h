#ifndef COMPOSITOR_ICON_CONVERT_H
#define COMPOSITOR_ICON_CONVERT_H

#include <cairo.h>
#include <xcb/xcb_ewmh.h>

struct wlr_buffer;

cairo_surface_t *icon_convert_buffer(struct wlr_buffer *buffer);
cairo_surface_t *icon_convert_ewmh(const xcb_ewmh_get_wm_icon_reply_t *reply, int wanted_size);

#endif
