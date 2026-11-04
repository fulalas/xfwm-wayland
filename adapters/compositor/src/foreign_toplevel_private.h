#ifndef COMPOSITOR_FOREIGN_TOPLEVEL_PRIVATE_H
#define COMPOSITOR_FOREIGN_TOPLEVEL_PRIVATE_H

#include <wayland-server-core.h>

#include "foreign_toplevel.h"

struct wlr_foreign_toplevel_handle_v1;

typedef struct toplevel_requests {
    struct wl_listener maximize;
    struct wl_listener minimize;
    struct wl_listener activate;
    struct wl_listener fullscreen;
    struct wl_listener close;
} toplevel_requests;

typedef struct toplevel_entry {
    foreign_toplevels *owner;
    view *view;
    struct wlr_foreign_toplevel_handle_v1 *wlr;
    struct wlr_ext_foreign_toplevel_handle_v1 *ext;
    toplevel_requests requests;
    struct wl_list link;
} toplevel_entry;

struct foreign_toplevels {
    compositor *server;
    struct wl_list entries;
    struct wl_listener mapped;
    struct wl_listener changed;
    struct wl_listener unmapped;
    struct wl_listener layout_change;
};

toplevel_entry *foreign_toplevels_find(const foreign_toplevels *toplevels, const view *view);
void toplevel_entry_sync(toplevel_entry *entry);
void toplevel_entry_hide(toplevel_entry *entry);
void toplevel_requests_connect(toplevel_entry *entry);
void toplevel_requests_disconnect(toplevel_entry *entry);

#endif
