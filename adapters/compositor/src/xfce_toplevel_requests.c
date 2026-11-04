#include <wlr/types/wlr_output.h>

#include "ext_workspace_resource.h"
#include "output.h"
#include "seat_resource.h"
#include "workspace_protocol.h"
#include "xfce_toplevel_private.h"

static void handle_set_shaded(struct wl_client *client, struct wl_resource *resource);
static void handle_unset_shaded(struct wl_client *client, struct wl_resource *resource);
static void handle_set_sticky(struct wl_client *client, struct wl_resource *resource);
static void handle_unset_sticky(struct wl_client *client, struct wl_resource *resource);
static void handle_set_above(struct wl_client *client, struct wl_resource *resource);
static void handle_unset_above(struct wl_client *client, struct wl_resource *resource);
static void handle_set_below(struct wl_client *client, struct wl_resource *resource);
static void handle_unset_below(struct wl_client *client, struct wl_resource *resource);
static void handle_move(struct wl_client *client, struct wl_resource *resource,
                        struct wl_resource *seat);
static void handle_resize(struct wl_client *client, struct wl_resource *resource,
                          struct wl_resource *seat);
static void handle_move_to_output(struct wl_client *client, struct wl_resource *resource,
                                  struct wl_resource *output);
static void handle_move_to_workspace(struct wl_client *client, struct wl_resource *resource,
                                     struct wl_resource *workspace);
static void handle_get_icon_pixels(struct wl_client *client, struct wl_resource *resource,
                                   uint32_t id, uint32_t size, uint32_t scale);
static void handle_destroy(struct wl_client *client, struct wl_resource *resource);
static void request_state(struct wl_resource *resource, wm_state_change change);
static wm_window *window_of(struct wl_resource *resource);
static compositor *server_of(struct wl_resource *resource);
static size_t output_index(const outputs *outputs, const struct wlr_output *wlr_output);

static const struct xfce_foreign_toplevel_handle_v1_interface handle_impl = {
    .set_shaded = handle_set_shaded,
    .unset_shaded = handle_unset_shaded,
    .set_sticky = handle_set_sticky,
    .unset_sticky = handle_unset_sticky,
    .set_above = handle_set_above,
    .unset_above = handle_unset_above,
    .set_below = handle_set_below,
    .unset_below = handle_unset_below,
    .highlight = xfce_handle_highlight,
    .unhighlight = xfce_handle_unhighlight,
    .move = handle_move,
    .resize = handle_resize,
    .move_to_output = handle_move_to_output,
    .move_to_workspace = handle_move_to_workspace,
    .get_icon_pixels = handle_get_icon_pixels,
    .destroy = handle_destroy,
};

const struct xfce_foreign_toplevel_handle_v1_interface *xfce_handle_requests(void)
{
    return &handle_impl;
}

static void handle_set_shaded(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    request_state(resource, (wm_state_change){WM_STATE_SHADED, 0});
}

static void handle_unset_shaded(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    request_state(resource, (wm_state_change){0, WM_STATE_SHADED});
}

static void handle_set_sticky(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    request_state(resource, (wm_state_change){WM_STATE_STICKY, 0});
}

static void handle_unset_sticky(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    request_state(resource, (wm_state_change){0, WM_STATE_STICKY});
}

static void handle_set_above(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    request_state(resource, (wm_state_change){WM_STATE_ABOVE, 0});
}

static void handle_unset_above(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    request_state(resource, (wm_state_change){0, WM_STATE_ABOVE});
}

static void handle_set_below(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    request_state(resource, (wm_state_change){WM_STATE_BELOW, 0});
}

static void handle_unset_below(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    request_state(resource, (wm_state_change){0, WM_STATE_BELOW});
}

static void handle_move(struct wl_client *client, struct wl_resource *resource,
                        struct wl_resource *seat)
{
    (void) client;
    wm_window *window = window_of(resource);
    compositor *server = server_of(resource);
    if (window != NULL && seat_resource_is_ours(server, seat)) {
        wm_request_keyboard_move(server->desktop, window);
    }
}

static void handle_resize(struct wl_client *client, struct wl_resource *resource,
                          struct wl_resource *seat)
{
    (void) client;
    wm_window *window = window_of(resource);
    compositor *server = server_of(resource);
    if (window != NULL && seat_resource_is_ours(server, seat)) {
        wm_request_keyboard_resize(server->desktop, window);
    }
}

static void handle_move_to_output(struct wl_client *client, struct wl_resource *resource,
                                  struct wl_resource *output)
{
    (void) client;
    wm_window *window = window_of(resource);
    compositor *server = server_of(resource);
    struct wlr_output *wlr_output = wlr_output_from_resource(output);
    if (window != NULL && wlr_output != NULL) {
        size_t index = output_index(server->outputs, wlr_output);
        wm_request_send_to_monitor(server->desktop, window, index);
    }
}

static void handle_move_to_workspace(struct wl_client *client, struct wl_resource *resource,
                                     struct wl_resource *workspace)
{
    (void) client;
    wm_window *window = window_of(resource);
    compositor *server = server_of(resource);
    const struct wlr_ext_workspace_handle_v1 *handle = ext_workspace_from_resource(workspace);
    int index = workspace_protocol_index(server->workspaces, handle);
    if (window != NULL && index >= 0) {
        wm_request_workspace(server->desktop, window, index);
    }
}

static void handle_get_icon_pixels(struct wl_client *client, struct wl_resource *resource,
                                   uint32_t id, uint32_t size, uint32_t scale)
{
    struct wl_resource *answer = xfce_icon_pixels_create(resource, id);
    if (answer == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    const xfce_handle *handle = wl_resource_get_user_data(resource);
    const view *v = handle->entry != NULL ? handle->entry->view : NULL;
    cairo_surface_t *pixels = v != NULL ? icon_slot_app_pixels(view_icon_slot(v)) : NULL;
    xfce_icon_pixels_answer(answer, pixels, (icon_size){size, scale});
}

static void handle_destroy(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    wl_resource_destroy(resource);
}

static void request_state(struct wl_resource *resource, wm_state_change change)
{
    wm_window *window = window_of(resource);
    if (window != NULL) {
        wm_request_state(server_of(resource)->desktop, window, &change);
    }
}

static wm_window *window_of(struct wl_resource *resource)
{
    const xfce_handle *handle = wl_resource_get_user_data(resource);
    return handle->entry != NULL ? view_window(handle->entry->view) : NULL;
}

static compositor *server_of(struct wl_resource *resource)
{
    const xfce_handle *handle = wl_resource_get_user_data(resource);
    return handle->toplevels->server;
}

static size_t output_index(const outputs *outs, const struct wlr_output *wlr_output)
{
    size_t count = outputs_count(outs);
    for (size_t i = 0; i < count; i++) {
        if (output_wlr(outputs_at_index(outs, i)) == wlr_output) {
            return i;
        }
    }
    return count;
}
