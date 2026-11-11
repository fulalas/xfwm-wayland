#ifndef COMPOSITOR_XDG_OUTPUT_H
#define COMPOSITOR_XDG_OUTPUT_H

#include "wm/geometry.h"

struct wl_client;
struct wl_display;
struct wlr_output_layout;

typedef struct xdg_output_scaler {
    void *self;
    wm_rect (*client_box)(void *self, struct wl_client *client, wm_rect logical);
} xdg_output_scaler;

typedef struct xdg_outputs xdg_outputs;

xdg_outputs *xdg_outputs_create(struct wl_display *display, struct wlr_output_layout *layout,
                                const xdg_output_scaler *scaler);
void xdg_outputs_refresh(xdg_outputs *outputs);

#endif
