#include <signal.h>
#include <unistd.h>
#include <wlr/types/wlr_scene.h>

#include "view_private.h"

static void place_below_lowest(view *view, struct wlr_scene_node **lowest);
static void kill_client_pid(pid_t pid);

void view_close(view *v)
{
    v->impl->close(v->self);
}

void view_kill(view *v)
{
    kill_client_pid(view_pid(v));
}

void view_focus(view *v)
{
    v->impl->focus(v->self);
}

void view_unfocus(view *v)
{
    v->impl->unfocus(v->self);
}

void views_stack(void *const *windows, size_t count)
{
    struct wlr_scene_node *lowest[SCENE_LAYER_COUNT] = {0};
    for (size_t i = count; i > 0; i--) {
        place_below_lowest(windows[i - 1], lowest);
    }
}

void views_invalidate_decorations(compositor *server)
{
    view *v;
    wl_list_for_each(v, &server->views, link) {
        v->paint_mark.painted = false;
        decorations_invalidate(v->decor);
    }
}

void views_reload_icons(compositor *server)
{
    view *v;
    wl_list_for_each(v, &server->views, link) {
        icon_slot_reload(v->impl->icon(v->self));
    }
}

static void place_below_lowest(view *v, struct wlr_scene_node **lowest)
{
    struct wlr_scene_node *node = &v->tree->node;
    struct wlr_scene_node *above = lowest[v->layer];
    if (above == NULL) {
        wlr_scene_node_raise_to_top(node);
    } else if (above != node) {
        wlr_scene_node_place_below(node, above);
    }
    lowest[v->layer] = node;
}

static void kill_client_pid(pid_t pid)
{
    if (pid > 0 && pid != getpid()) {
        kill(pid, SIGKILL);
    }
}
