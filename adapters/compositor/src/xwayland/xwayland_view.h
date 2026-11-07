#ifndef COMPOSITOR_XWAYLAND_VIEW_H
#define COMPOSITOR_XWAYLAND_VIEW_H

#include "server/server.h"
#include "output/usable_area.h"

struct wlr_xwayland_surface;

typedef struct xwayland_view xwayland_view;

xwayland_view *xwayland_view_create(compositor *server, struct wlr_xwayland_surface *xsurface,
                                    struct wl_list *views);
wm_rect xwayland_views_apply_struts(const struct wl_list *views, const strut_context *context,
                                    wm_rect usable);

#endif
