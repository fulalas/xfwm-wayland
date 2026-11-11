#ifndef COMPOSITOR_XWAYLAND_H
#define COMPOSITOR_XWAYLAND_H

#include <stdbool.h>

#include "output/output.h"
#include "server/server.h"
#include "wm/views.h"
#include "x11_link.h"

struct wl_client;
struct wlr_scene_node;
struct wlr_scene_tree;
struct wlr_surface;
struct wlr_xwayland_surface;

xwayland_shell *xwayland_shell_create(compositor *server);
void xwayland_shell_destroy(xwayland_shell *shell);
const char *xwayland_shell_display_name(const xwayland_shell *shell);
wm_rect xwayland_shell_apply_struts(const xwayland_shell *shell, const output *output,
                                    wm_rect usable);
void xwayland_shell_publish_workspaces(xwayland_shell *shell, const wm_workspaces_view *view);
void xwayland_shell_outputs_arranged(xwayland_shell *shell);
const x11_link *xwayland_shell_link(const xwayland_shell *shell);
struct wl_client *xwayland_shell_client(const xwayland_shell *shell);
bool xwayland_shell_grabs_keyboard(const xwayland_shell *shell, struct wlr_surface *focused);
void xwayland_shell_update_scale(xwayland_shell *shell);
double xwayland_shell_scale(const xwayland_shell *shell);
wm_rect xwayland_shell_logical_rect(const xwayland_shell *shell,
                                    const struct wlr_xwayland_surface *xsurface);
struct wlr_scene_tree *xwayland_surface_tree(compositor *server, struct wlr_scene_tree *parent,
                                             struct wlr_surface *surface);
double xwayland_client_scale(const compositor *server, const struct wl_client *client);
double xwayland_surface_scale(const compositor *server, const struct wlr_surface *surface);
void xwayland_node_local(const compositor *server, struct wlr_scene_node *node, double lx,
                         double ly, double *sx, double *sy);

#endif
