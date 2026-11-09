#ifndef COMPOSITOR_XDG_SHELL_H
#define COMPOSITOR_XDG_SHELL_H

#include "server/server.h"

struct wlr_seat_keyboard_grab;
struct wlr_surface;

xdg_shell *xdg_shell_create(compositor *server);
void xdg_shell_destroy(xdg_shell *shell);
struct wlr_surface *xdg_shell_popup_grabbed(const xdg_shell *shell,
                                            const struct wlr_seat_keyboard_grab *grab);

#endif
