#include "accessibility.h"
#include "shell/activation.h"
#include "input/device_list.h"
#include "output/displays.h"
#include "output/drm_lease.h"
#include "toplevel/foreign_toplevel.h"
#include "input/idle.h"
#include "shell/layer_shell.h"
#include "input/lid_switch.h"
#include "lock.h"
#include "osd/osd.h"
#include "output/output.h"
#include "output/output_config.h"
#include "output/output_power.h"
#include "render/present_sync.h"
#include "protocols.h"
#include "render/render_sync.h"
#include "input/seat.h"
#include "server_modules.h"
#include "toplevel/toplevel_capture.h"
#include "toplevel/toplevel_icons.h"
#include "output/xfce_output_heads.h"
#include "output/xfce_outputs.h"
#include "workspace/workspace_protocol.h"
#include "session/xdg_session.h"
#include "shell/xdg_shell.h"
#include "xwayland/xwayland.h"
#include "output/zoom.h"

#define DESTROY_AND_CLEAR(pointer, destroy) \
    do {                                    \
        if ((pointer) != NULL) {            \
            destroy(pointer);               \
            (pointer) = NULL;               \
        }                                   \
    } while (0)

static bool create_core_modules(compositor *server);
static void *require(bool *ready, void *module);
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
    bool core_ready = create_core_modules(c);
    c->socket = wl_display_add_socket_auto(c->display);
    return core_ready && create_desktop_modules(c) && c->socket != NULL;
}

static bool create_core_modules(compositor *c)
{
    bool ready = true;
    c->outputs = outputs_create(c);
    c->seat = require(&ready, c->outputs != NULL ? seat_create(c) : NULL);
    c->device_list = device_list_create(c);
    c->osd = require(&ready, osd_create(c));
    c->zoom = require(&ready, zoom_create(c));
    c->idle = require(&ready, idle_create(c));
    c->xdg = require(&ready, xdg_shell_create(c));
    c->layer_shell = require(&ready, layer_shell_create(c));
    c->lock = require(&ready, session_lock_create(c));
    c->toplevel_icons = require(&ready, toplevel_icons_create(c));
    c->xwayland = require(&ready, c->seat != NULL ? xwayland_shell_create(c) : NULL);
    return ready;
}

static void *require(bool *ready, void *module)
{
    *ready = *ready && module != NULL;
    return module;
}

static bool create_desktop_modules(compositor *c)
{
    bool ready = true;
    c->foreign_toplevels = require(&ready, foreign_toplevels_create(c));
    c->toplevel_capture = require(&ready, toplevel_capture_create(c));
    c->workspaces = require(&ready, workspace_protocol_create(c));
    c->activation = require(&ready, activation_create(c));
    c->output_config = require(&ready, output_config_create(c));
    c->output_power = require(&ready, output_power_create(c));
    ready = create_display_modules(c) && ready;
    c->drm_lease = require(&ready, drm_lease_create(c));
    c->xdg_sessions = require(&ready, xdg_sessions_create(c));
    c->present_sync = require(&ready, present_sync_create(c));
    c->accessibility = require(&ready, accessibility_create(c));
    return ready;
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
    bool ready = true;
    c->displays = require(&ready, displays_create(c));
    c->lid_switch = require(&ready, lid_switch_create(c));
    c->xfce_output_heads = require(&ready, xfce_output_heads_create(c));
    c->xfce_outputs = require(&ready, xfce_outputs_create(c));
    return ready;
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
