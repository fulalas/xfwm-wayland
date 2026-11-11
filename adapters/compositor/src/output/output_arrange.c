#include <wlr/types/wlr_output.h>

#include "shell/layer_shell.h"
#include "server/lock.h"
#include "output_private.h"
#include "input/seat.h"
#include "xfce_outputs.h"
#include "xwayland/xwayland.h"

static bool output_update_usable(output *output);
static void update_max_scale(outputs *outputs);

void outputs_arrange(outputs *outs)
{
    compositor *server = outs->server;
    bool changed = outs->changed;
    outs->changed = false;
    output *o;
    wl_list_for_each(o, &outs->list, link) {
        changed |= output_update_usable(o);
    }
    if (server->lock != NULL) {
        session_lock_arrange(server->lock);
    }
    update_max_scale(outs);
    if (changed) {
        wm_desktop_outputs_changed(server->desktop);
    }
    xfce_outputs_refresh(server->xfce_outputs);
    if (server->xwayland != NULL) {
        xwayland_shell_outputs_arranged(server->xwayland);
    }
}

static bool output_update_usable(output *o)
{
    compositor *server = o->server;
    wm_rect usable = output_full(o);
    if (server->layer_shell != NULL) {
        usable = layer_shell_arrange(server->layer_shell, o);
    }
    if (server->xwayland != NULL) {
        usable = xwayland_shell_apply_struts(server->xwayland, o, usable);
    }
    bool changed = !wm_rect_equals(&usable, &o->usable);
    o->usable = usable;
    return changed;
}

static void update_max_scale(outputs *outs)
{
    double max_scale = 1.0;
    bool any_enabled = false;
    output *o;
    wl_list_for_each(o, &outs->list, link) {
        any_enabled |= o->wlr_output->enabled;
        if (o->wlr_output->enabled && o->wlr_output->scale > max_scale) {
            max_scale = o->wlr_output->scale;
        }
    }
    if (!any_enabled || max_scale == outs->max_scale) {
        return;
    }
    outs->max_scale = max_scale;
    if (outs->server->seat != NULL) {
        seat_refresh_cursor(outs->server->seat);
    }
    if (outs->server->xwayland != NULL) {
        xwayland_shell_update_scale(outs->server->xwayland);
    }
    const compositor_listener *listener = outs->server->listener;
    if (listener != NULL && listener->output_scale_changed != NULL) {
        listener->output_scale_changed(listener->self, max_scale);
    }
}
