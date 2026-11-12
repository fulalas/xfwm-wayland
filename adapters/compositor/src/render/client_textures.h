#ifndef COMPOSITOR_CLIENT_TEXTURES_H
#define COMPOSITOR_CLIENT_TEXTURES_H

struct wl_display;
struct wlr_renderer;

void client_textures_move(struct wl_display *display, struct wlr_renderer *renderer);

#endif
