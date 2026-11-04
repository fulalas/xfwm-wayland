#include "output.h"
#include "view_private.h"

struct wlr_scene_tree *view_content_tree(const view *v)
{
    return v->content;
}

struct wlr_scene_tree *view_popup_tree(const view *v)
{
    return v->popups;
}

wm_window *view_window(const view *v)
{
    return v->window;
}

struct wlr_surface *view_surface(const view *v)
{
    return v->impl->surface(v->self);
}

cairo_surface_t *view_icon(const view *v)
{
    return icon_slot_surface(v->impl->icon(v->self));
}

const icon_slot *view_icon_slot(const view *v)
{
    return v->impl->icon(v->self);
}

const char *view_title(const view *v)
{
    return v->target.title;
}

wm_rect view_client_rect(const view *v)
{
    return v->target.state.client;
}

wm_rect view_output_area(const view *v)
{
    wm_rect client = v->target.state.client;
    output *o = outputs_for_rect(v->server->outputs, &client);
    return o != NULL ? output_usable(o) : client;
}

bool view_is_fullscreen(const view *v)
{
    return v->target.state.fullscreen;
}

pid_t view_pid(const view *v)
{
    return v->impl->pid(v->self);
}

const wm_window_view *view_state(const view *v)
{
    return &v->target.state;
}

view *views_find_by_surface(compositor *server, struct wlr_surface *surface)
{
    view *v;
    wl_list_for_each(v, &server->views, link) {
        if (surface != NULL && view_surface(v) == surface) {
            return v;
        }
    }
    return NULL;
}
