#ifndef COMPOSITOR_XWAYLAND_EWMH_H
#define COMPOSITOR_XWAYLAND_EWMH_H

#include <stdbool.h>
#include <stdint.h>

#include "server.h"
#include "wm/views.h"
#include "x11_link.h"

typedef struct xwayland_ewmh xwayland_ewmh;

xwayland_ewmh *xwayland_ewmh_create(compositor *server, const x11_link *link);
void xwayland_ewmh_destroy(xwayland_ewmh *ewmh);
void xwayland_ewmh_link_ready(xwayland_ewmh *ewmh);
void xwayland_ewmh_publish_workspaces(xwayland_ewmh *ewmh, const wm_workspaces_view *view);
void xwayland_ewmh_publish_outputs(xwayland_ewmh *ewmh);
bool xwayland_ewmh_can_switch_to(const xwayland_ewmh *ewmh, uint32_t desktop);

#endif
