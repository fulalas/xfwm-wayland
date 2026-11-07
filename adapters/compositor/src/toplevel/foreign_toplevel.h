#ifndef COMPOSITOR_FOREIGN_TOPLEVEL_H
#define COMPOSITOR_FOREIGN_TOPLEVEL_H

#include "server/server.h"
#include "shell/view.h"

struct wlr_ext_foreign_toplevel_handle_v1;

foreign_toplevels *foreign_toplevels_create(compositor *server);
void foreign_toplevels_destroy(foreign_toplevels *toplevels);
void foreign_toplevels_sync_all(foreign_toplevels *toplevels);
view *foreign_toplevel_ext_view(const struct wlr_ext_foreign_toplevel_handle_v1 *handle);

#endif
