#include <stdlib.h>
#include <wlr/render/drm_syncobj.h>
#include <wlr/render/swapchain.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_damage_ring.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>

#include "output_private.h"
#include "zoom.h"
#include "zoom_private.h"

static const double ZOOM_EPSILON = 1e-3;

static void mark_view_changed(zoom *zoom);
static void zoom_leave(zoom *zoom);

zoom *zoom_create(compositor *server)
{
    zoom *z = calloc(1, sizeof(*z));
    if (z == NULL) {
        return NULL;
    }
    z->server = server;
    z->view.level = 1.0;
    return z;
}

void zoom_destroy(zoom *z)
{
    free(z);
}

void zoom_set(zoom *z, const wm_zoom_view *view)
{
    bool was_active = z->active;
    z->view = *view;
    z->active = view->level > 1.0 + ZOOM_EPSILON;
    if (was_active && !z->active) {
        zoom_leave(z);
    }
    mark_view_changed(z);
    outputs_schedule_frames(z->server->outputs);
}

bool zoom_is_active(const zoom *z)
{
    return z != NULL && z->active;
}

void zoom_release(output *o)
{
    zoom_target *target = &o->zoom;
    if (target->source != NULL) {
        wlr_buffer_unlock(target->source);
        target->source = NULL;
    }
    if (target->swapchain != NULL) {
        wlr_swapchain_destroy(target->swapchain);
        target->swapchain = NULL;
    }
    wlr_drm_syncobj_timeline_unref(target->timeline);
    target->timeline = NULL;
    target->point = 0;
    if (target->cursor_locked) {
        wlr_output_lock_software_cursors(o->wlr_output, false);
        target->cursor_locked = false;
    }
}

static void mark_view_changed(zoom *z)
{
    outputs *outs = z->server->outputs;
    for (output *o = outputs_next_active(outs, NULL); o != NULL; o = outputs_next_active(outs, o)) {
        o->zoom.view_changed = true;
    }
}

static void zoom_leave(zoom *z)
{
    outputs *outs = z->server->outputs;
    for (output *o = outputs_next_active(outs, NULL); o != NULL; o = outputs_next_active(outs, o)) {
        zoom_release(o);
        if (o->scene_output != NULL) {
            wlr_damage_ring_add_whole(&o->scene_output->damage_ring);
        }
    }
}
