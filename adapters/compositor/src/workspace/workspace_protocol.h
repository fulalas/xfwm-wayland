#ifndef COMPOSITOR_WORKSPACE_PROTOCOL_H
#define COMPOSITOR_WORKSPACE_PROTOCOL_H

#include "server/server.h"
#include "wm/views.h"

struct wlr_ext_workspace_handle_v1;

workspace_protocol *workspace_protocol_create(compositor *server);
void workspace_protocol_destroy(workspace_protocol *workspaces);
void workspace_protocol_publish(workspace_protocol *workspaces, const wm_workspaces_view *view);
struct wlr_ext_workspace_handle_v1 *workspace_protocol_handle(const workspace_protocol *workspaces,
                                                              unsigned id);
int workspace_protocol_index(const workspace_protocol *workspaces,
                             const struct wlr_ext_workspace_handle_v1 *handle);

#endif
