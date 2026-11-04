#ifndef COMPOSITOR_WORKSPACE_PROTOCOL_H
#define COMPOSITOR_WORKSPACE_PROTOCOL_H

#include "server.h"
#include "wm/views.h"

workspace_protocol *workspace_protocol_create(compositor *server);
void workspace_protocol_destroy(workspace_protocol *workspaces);
void workspace_protocol_publish(workspace_protocol *workspaces, const wm_workspaces_view *view);

#endif
