#ifndef COMPOSITOR_KEYBOARD_GRABS_H
#define COMPOSITOR_KEYBOARD_GRABS_H

#include <stdbool.h>

#include "server/server.h"

struct wl_display;
struct wlr_surface;

typedef struct keyboard_grabs keyboard_grabs;

keyboard_grabs *keyboard_grabs_create(struct wl_display *display, xwayland_shell *shell);
void keyboard_grabs_destroy(keyboard_grabs *grabs);
bool keyboard_grabs_hold(const keyboard_grabs *grabs, struct wlr_surface *focused);

#endif
