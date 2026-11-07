#ifndef COMPOSITOR_XDG_SESSION_H
#define COMPOSITOR_XDG_SESSION_H

#include "server/server.h"
#include "wm/desktop.h"

struct wlr_xdg_toplevel;

xdg_sessions *xdg_sessions_create(compositor *server);
void xdg_sessions_destroy(xdg_sessions *sessions);
wm_size xdg_sessions_initial_size(const xdg_sessions *sessions,
                                  const struct wlr_xdg_toplevel *toplevel);
void xdg_sessions_apply_restore(xdg_sessions *sessions, const struct wlr_xdg_toplevel *toplevel,
                           wm_window_desc *desc);

#endif
