#ifndef COMPOSITOR_LAYER_SHELL_H
#define COMPOSITOR_LAYER_SHELL_H

#include <stdbool.h>

#include "output.h"
#include "server.h"

struct wlr_surface;

typedef struct layer_surface layer_surface;

layer_shell *layer_shell_create(compositor *server);
void layer_shell_destroy(layer_shell *shell);
wm_rect layer_shell_arrange(layer_shell *shell, output *output);
struct wlr_surface *layer_shell_exclusive_focus(const layer_shell *shell);
void layer_shell_output_destroyed(layer_shell *shell, output *output);

bool layer_surface_accepts_click_focus(const layer_surface *surface);
struct wlr_surface *layer_surface_wlr_surface(const layer_surface *surface);

#endif
