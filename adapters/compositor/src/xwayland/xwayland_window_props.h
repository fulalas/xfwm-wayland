#ifndef COMPOSITOR_XWAYLAND_WINDOW_PROPS_H
#define COMPOSITOR_XWAYLAND_WINDOW_PROPS_H

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

#include "ewmh_payload.h"
#include "x11_link.h"

typedef struct x11_window_props {
    bool published;
    uint32_t desktop;
    ewmh_frame_extents extents;
    uint32_t actions;
} x11_window_props;

typedef struct x11_window_target {
    const x11_link *link;
    xcb_window_t window;
    double scale;
} x11_window_target;

void x11_window_props_publish(x11_window_props *last, const x11_window_target *target,
                              const wm_window_view *state);
void x11_window_props_publish_extents(const x11_window_target *target,
                                      const ewmh_frame_extents *extents);
ewmh_frame_extents x11_window_props_extents(const x11_window_target *target,
                                            const wm_extents *logical);

#endif
