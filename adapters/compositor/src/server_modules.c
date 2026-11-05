#include "accessibility.h"
#include "activation.h"
#include "device_list.h"
#include "displays.h"
#include "drm_lease.h"
#include "foreign_toplevel.h"
#include "idle.h"
#include "layer_shell.h"
#include "lid_switch.h"
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
#include "xfce_output_heads.h"
#include "xfce_outputs.h"
#include "workspace_protocol.h"
#include "xdg_session.h"
#include "xdg_shell.h"
#include "xwayland.h"
#include "zoom.h"

#define DESTROY_AND_CLEAR(pointer, destroy) \
    do {                                    \
        if ((pointer) != NULL) {            \
            destroy(pointer);               \
            (pointer) = NULL;               \
        }                                   \
    } while (0)

static bool create_desktop_modules(compositor *server);
static bool create_display_modules(compositor *server);
static void destroy_client_shells(compositor *server);
static void destroy_desktop_modules(compositor *server);
static void destroy_output_modules(compositor *server);
static void destroy_display_modules(compositor *server);

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
    bool displays_created = create_display_modules(c);
    c->drm_lease = drm_lease_create(c);
    c->xdg_sessions = xdg_sessions_create(c);
    c->present_sync = present_sync_create(c);
    c->accessibility = accessibility_create(c);
    return c->foreign_toplevels != NULL && c->toplevel_capture != NULL && c->workspaces != NULL &&
           c->activation != NULL && c->output_config != NULL && c->output_power != NULL &&
           displays_created && c->drm_lease != NULL && c->xdg_sessions != NULL &&
           c->present_sync != NULL && c->accessibility != NULL;
}

void server_modules_destroy(compositor *c)
{
    destroy_client_shells(c);
    destroy_desktop_modules(c);
    DESTROY_AND_CLEAR(c->idle, idle_destroy);
    DESTROY_AND_CLEAR(c->zoom, zoom_destroy);
    DESTROY_AND_CLEAR(c->osd, osd_destroy);
    DESTROY_AND_CLEAR(c->device_list, device_list_destroy);
    DESTROY_AND_CLEAR(c->seat, seat_destroy);
    DESTROY_AND_CLEAR(c->outputs, outputs_destroy);
}

static bool create_display_modules(compositor *c)
{
    c->displays = displays_create(c);
    c->lid_switch = lid_switch_create(c);
    c->xfce_output_heads = xfce_output_heads_create(c);
    c->xfce_outputs = xfce_outputs_create(c);
    return c->displays != NULL && c->lid_switch != NULL && c->xfce_output_heads != NULL &&
           c->xfce_outputs != NULL;
}

static void destroy_client_shells(compositor *c)
{
    DESTROY_AND_CLEAR(c->xwayland, xwayland_shell_destroy);
    DESTROY_AND_CLEAR(c->toplevel_icons, toplevel_icons_destroy);
    DESTROY_AND_CLEAR(c->lock, session_lock_destroy);
    DESTROY_AND_CLEAR(c->layer_shell, layer_shell_destroy);
    DESTROY_AND_CLEAR(c->xdg, xdg_shell_destroy);
}

static void destroy_desktop_modules(compositor *c)
{
    destroy_output_modules(c);
    DESTROY_AND_CLEAR(c->accessibility, accessibility_destroy);
    DESTROY_AND_CLEAR(c->activation, activation_destroy);
    DESTROY_AND_CLEAR(c->workspaces, workspace_protocol_destroy);
    DESTROY_AND_CLEAR(c->toplevel_capture, toplevel_capture_destroy);
    DESTROY_AND_CLEAR(c->foreign_toplevels, foreign_toplevels_destroy);
}

static void destroy_output_modules(compositor *c)
{
    destroy_display_modules(c);
    DESTROY_AND_CLEAR(c->present_sync, present_sync_destroy);
    DESTROY_AND_CLEAR(c->drm_lease, drm_lease_destroy);
    DESTROY_AND_CLEAR(c->xdg_sessions, xdg_sessions_destroy);
    DESTROY_AND_CLEAR(c->output_power, output_power_destroy);
    DESTROY_AND_CLEAR(c->output_config, output_config_destroy);
}

static void destroy_display_modules(compositor *c)
{
    DESTROY_AND_CLEAR(c->xfce_outputs, xfce_outputs_destroy);
    DESTROY_AND_CLEAR(c->xfce_output_heads, xfce_output_heads_destroy);
    DESTROY_AND_CLEAR(c->lid_switch, lid_switch_destroy);
    DESTROY_AND_CLEAR(c->displays, displays_destroy);
}
