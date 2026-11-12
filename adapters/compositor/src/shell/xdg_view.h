#ifndef COMPOSITOR_XDG_VIEW_H
#define COMPOSITOR_XDG_VIEW_H

#include "toplevel/icon_slot.h"
#include "server/server.h"

struct wlr_xdg_toplevel;
struct wlr_xdg_toplevel_decoration_v1;
struct wlr_server_decoration;

typedef struct xdg_view xdg_view;

xdg_view *xdg_view_create(compositor *server, struct wlr_xdg_toplevel *toplevel);
xdg_view *xdg_view_from_toplevel(struct wlr_xdg_toplevel *toplevel);
void xdg_view_attach_decoration(xdg_view *view, struct wlr_xdg_toplevel_decoration_v1 *decoration);
void xdg_view_attach_kde_decoration(xdg_view *view, struct wlr_server_decoration *decoration);
void xdg_view_modal_changed(xdg_view *view, bool modal);
void xdg_view_parent_requested(xdg_view *view, xdg_view *parent);
icon_slot *xdg_view_icon_slot(const xdg_view *view);
const char *xdg_view_app_id(const xdg_view *view);

#endif
