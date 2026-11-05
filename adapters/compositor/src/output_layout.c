#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "listener.h"
#include "output_private.h"

static bool connect_scene(output *output, struct wlr_output_layout_output *layout_output);
static void handle_scene_destroy(struct wl_listener *listener, void *data);

size_t outputs_count(const outputs *outs)
{
    size_t count = 0;
    output *o;
    wl_list_for_each(o, &outs->list, link) {
        count += output_is_active(o) ? 1 : 0;
    }
    return count;
}

output *outputs_at_index(const outputs *outs, size_t index)
{
    size_t i = 0;
    output *o;
    wl_list_for_each(o, &outs->list, link) {
        if (output_is_active(o) && i++ == index) {
            return o;
        }
    }
    return NULL;
}

output *outputs_next_active(const outputs *outs, const output *after)
{
    const struct wl_list *link = after != NULL ? after->link.next : outs->list.next;
    for (; link != &outs->list; link = link->next) {
        output *o = wl_container_of(link, o, link);
        if (output_is_active(o)) {
            return o;
        }
    }
    return NULL;
}

size_t outputs_known_count(const outputs *outs)
{
    return (size_t) wl_list_length(&outs->list);
}

output *outputs_known_at(const outputs *outs, size_t index)
{
    size_t i = 0;
    output *o;
    wl_list_for_each(o, &outs->list, link) {
        if (i++ == index) {
            return o;
        }
    }
    return NULL;
}

bool output_is_active(const output *o)
{
    return wlr_output_layout_get(o->server->layout, o->wlr_output) != NULL;
}

bool output_attach_auto(output *o)
{
    struct wlr_output_layout_output *layout_output =
        wlr_output_layout_add_auto(o->server->layout, o->wlr_output);
    if (layout_output == NULL || !connect_scene(o, layout_output)) {
        return false;
    }
    o->usable = output_full(o);
    return true;
}

bool output_place(output *o, wm_point position)
{
    bool was_active = output_is_active(o);
    struct wlr_output_layout_output *layout_output =
        wlr_output_layout_add(o->server->layout, o->wlr_output, position.x, position.y);
    if (layout_output == NULL) {
        wlr_log(WLR_ERROR, "cannot place output %s in the layout", o->wlr_output->name);
        return false;
    }
    return was_active || connect_scene(o, layout_output);
}

void output_remove_from_layout(output *o)
{
    if (output_is_active(o)) {
        wlr_output_layout_remove(o->server->layout, o->wlr_output);
    }
}

struct wlr_scene_output *output_scene(output *o)
{
    if (o->scene_output == NULL) {
        o->scene_output = wlr_scene_output_create(o->server->scene, o->wlr_output);
        if (o->scene_output != NULL) {
            listener_connect(&o->scene_destroy, &o->scene_output->events.destroy,
                             handle_scene_destroy);
        }
    }
    return o->scene_output;
}

static bool connect_scene(output *o, struct wlr_output_layout_output *layout_output)
{
    struct wlr_scene_output *scene_output = output_scene(o);
    if (scene_output == NULL) {
        return false;
    }
    wlr_scene_output_layout_add_output(o->server->scene_layout, layout_output, scene_output);
    return true;
}

static void handle_scene_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    output *o = wl_container_of(listener, o, scene_destroy);
    listener_disconnect(&o->scene_destroy);
    o->scene_output = NULL;
    o->scene_hdr = false;
}
