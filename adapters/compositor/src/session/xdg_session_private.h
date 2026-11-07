#ifndef COMPOSITOR_XDG_SESSION_PRIVATE_H
#define COMPOSITOR_XDG_SESSION_PRIVATE_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>

#include "session_book.h"
#include "xdg_session.h"

struct wlr_xdg_toplevel;

struct xdg_sessions {
    compositor *server;
    struct wl_global *global;
    struct wl_list sessions;
    struct wl_listener view_changed;
};

typedef struct session_data {
    xdg_sessions *owner;
    struct wl_resource *resource;
    char *id;
    uint32_t reason;
    session_book book;
    struct wl_list toplevels;
    struct wl_event_source *save_timer;
    struct wl_list link;
} session_data;

typedef struct session_toplevel {
    session_data *session;
    struct wl_resource *resource;
    struct wlr_xdg_toplevel *toplevel;
    char *name;
    bool restore_pending;
    session_record restore;
    struct wl_listener toplevel_destroy;
    struct wl_listener surface_commit;
    struct wl_list link;
} session_toplevel;

typedef struct toplevel_request {
    struct wl_resource *session;
    uint32_t id;
    struct wl_resource *toplevel;
    const char *name;
} toplevel_request;

void session_data_changed(session_data *session);

void session_toplevel_add(const toplevel_request *request);
void session_toplevel_restore(const toplevel_request *request);
void session_toplevel_forget(session_data *session, const char *name);
void session_toplevel_make_inert(session_toplevel *toplevel);
session_toplevel *session_toplevel_of(const xdg_sessions *sessions,
                                      const struct wlr_xdg_toplevel *toplevel);
void session_toplevel_store(session_toplevel *toplevel, const wm_window_view *view);

#endif
