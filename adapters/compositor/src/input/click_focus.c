#include "click_focus.h"
#include "shell/layer_shell.h"
#include "seat_private.h"

void click_focus(seat *s, const pointer_hit *hit)
{
    if (hit->owner == NULL) {
        return;
    }
    node_role role = hit->owner->role;
    if (role == NODE_DESKTOP_SURFACE && s->focus.view != NULL) {
        return;
    }
    if (role == NODE_LAYER_SURFACE || role == NODE_DESKTOP_SURFACE) {
        if (layer_surface_accepts_click_focus(hit->owner->object)) {
            seat_focus_layer(s, layer_surface_wlr_surface(hit->owner->object));
        }
        return;
    }
    if (role == NODE_VIEW_CLIENT || role == NODE_VIEW_FRAME) {
        seat_focus_clicked_view(s);
    }
}
