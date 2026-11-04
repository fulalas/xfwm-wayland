#include "accessibility.h"
#include "activation.h"
#include "display_modules.h"
#include "device_list.h"
#include "drm_lease.h"
#include "foreign_toplevel.h"
#include "idle.h"
#include "layer_shell.h"
#include "lock.h"
#include "osd.h"
#include "output.h"
#include "output_config.h"
#include "output_power.h"
#include "present_sync.h"
#include "protocols.h"
#include "render_sync.h"
#include "seat.h"
#include "server_modules.h"
#include "toplevel_capture.h"
#include "toplevel_icons.h"
#include "workspace_protocol.h"
#include "xdg_session.h"
#include "xdg_shell.h"
#include "xwayland.h"
#include "zoom.h"

static bool create_desktop_modules(compositor *server);
static void destroy_client_shells(compositor *server);
static void destroy_desktop_modules(compositor *server);
static void destroy_output_modules(compositor *server);
static void destroy_input_modules(compositor *server);

bool server_modules_create(compositor *c)
{
    if (!protocols_create(c) || !protocols_create_input(c) || !protocols_create_desktop(c)) {
        return false;
    }
    protocols_restrict_sandboxed(c);
    render_sync_create_global(c);
    c->outputs = outputs_create(c);
    c->seat = c->outputs != NULL ? seat_create(c) : NULL;
    c->device_list = device_list_create(c);
    c->osd = osd_create(c);
    c->zoom = zoom_create(c);
    c->idle = idle_create(c);
    c->xdg = xdg_shell_create(c);
    c->layer_shell = layer_shell_create(c);
    c->lock = session_lock_create(c);
    c->toplevel_icons = toplevel_icons_create(c);
    c->xwayland = c->seat != NULL ? xwayland_shell_create(c) : NULL;
    c->socket = wl_display_add_socket_auto(c->display);
    return c->seat != NULL && c->osd != NULL && c->zoom != NULL && c->idle != NULL &&
           c->xdg != NULL && c->layer_shell != NULL && c->lock != NULL &&
           c->toplevel_icons != NULL && c->xwayland != NULL && create_desktop_modules(c) &&
           c->socket != NULL;
}

static bool create_desktop_modules(compositor *c)
{
    c->foreign_toplevels = foreign_toplevels_create(c);
    c->toplevel_capture = toplevel_capture_create(c);
    c->workspaces = workspace_protocol_create(c);
    c->activation = activation_create(c);
    c->output_config = output_config_create(c);
    c->output_power = output_power_create(c);
    bool displays_created = display_modules_create(c);
    c->drm_lease = drm_lease_create(c);
    c->xdg_sessions = xdg_sessions_create(c);
    c->present_sync = present_sync_create(c);
    c->accessibility = accessibility_create(c);
    return c->foreign_toplevels != NULL && c->toplevel_capture != NULL && c->workspaces != NULL &&
           c->activation != NULL && c->output_config != NULL && c->output_power != NULL &&
           displays_created &&
           c->drm_lease != NULL && c->xdg_sessions != NULL && c->present_sync != NULL && c->accessibility != NULL;
}

void server_modules_destroy(compositor *c)
{
    destroy_client_shells(c);
    destroy_desktop_modules(c);
    if (c->idle != NULL) {
        idle_destroy(c->idle);
        c->idle = NULL;
    }
    if (c->zoom != NULL) {
        zoom_destroy(c->zoom);
        c->zoom = NULL;
    }
    if (c->osd != NULL) {
        osd_destroy(c->osd);
        c->osd = NULL;
    }
    destroy_input_modules(c);
    if (c->seat != NULL) {
        seat_destroy(c->seat);
        c->seat = NULL;
    }
    if (c->outputs != NULL) {
        outputs_destroy(c->outputs);
        c->outputs = NULL;
    }
}

static void destroy_client_shells(compositor *c)
{
    if (c->xwayland != NULL) {
        xwayland_shell_destroy(c->xwayland);
        c->xwayland = NULL;
    }
    if (c->toplevel_icons != NULL) {
        toplevel_icons_destroy(c->toplevel_icons);
        c->toplevel_icons = NULL;
    }
    if (c->lock != NULL) {
        session_lock_destroy(c->lock);
        c->lock = NULL;
    }
    if (c->layer_shell != NULL) {
        layer_shell_destroy(c->layer_shell);
        c->layer_shell = NULL;
    }
    if (c->xdg != NULL) {
        xdg_shell_destroy(c->xdg);
        c->xdg = NULL;
    }
}

static void destroy_desktop_modules(compositor *c)
{
    destroy_output_modules(c);
    if (c->accessibility != NULL) {
        accessibility_destroy(c->accessibility);
        c->accessibility = NULL;
    }
    if (c->activation != NULL) {
        activation_destroy(c->activation);
        c->activation = NULL;
    }
    if (c->workspaces != NULL) {
        workspace_protocol_destroy(c->workspaces);
        c->workspaces = NULL;
    }
    if (c->toplevel_capture != NULL) {
        toplevel_capture_destroy(c->toplevel_capture);
        c->toplevel_capture = NULL;
    }
    if (c->foreign_toplevels != NULL) {
        foreign_toplevels_destroy(c->foreign_toplevels);
        c->foreign_toplevels = NULL;
    }
}

static void destroy_output_modules(compositor *c)
{
    display_modules_destroy(c);
    if (c->present_sync != NULL) {
        present_sync_destroy(c->present_sync);
        c->present_sync = NULL;
    }
    if (c->drm_lease != NULL) {
        drm_lease_destroy(c->drm_lease);
        c->drm_lease = NULL;
    }
    if (c->xdg_sessions != NULL) {
        xdg_sessions_destroy(c->xdg_sessions);
        c->xdg_sessions = NULL;
    }
    if (c->output_power != NULL) {
        output_power_destroy(c->output_power);
        c->output_power = NULL;
    }
    if (c->output_config != NULL) {
        output_config_destroy(c->output_config);
        c->output_config = NULL;
    }
}

static void destroy_input_modules(compositor *c)
{
    if (c->device_list != NULL) {
        device_list_destroy(c->device_list);
        c->device_list = NULL;
    }
}
