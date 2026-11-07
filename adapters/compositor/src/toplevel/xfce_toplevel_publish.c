#include <wayland-server-core.h>

#include "workspace/ext_workspace_resource.h"
#include "util/listener.h"
#include "wlr-foreign-toplevel-management-unstable-v1-protocol.h"
#include "workspace/workspace_protocol.h"
#include "xfce_toplevel_private.h"

enum {
    STATE_COUNT = XFCE_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_DEMANDS_ATTENTION + 1,
};

static xfce_published wanted_of(const xfce_handle *handle);
static bool is_published(const xfce_published *current, const xfce_published *wanted);
static uint32_t wanted_states(const wm_window_view *state);
static uint32_t state_bit(enum xfce_foreign_toplevel_handle_v1_state state, bool enabled);
static struct wl_resource *wanted_workspace(const xfce_handle *handle,
                                            const wm_window_view *state);
static void send_states(xfce_handle *handle, uint32_t states);
static void append_state(struct wl_array *array, uint32_t state);
static void send_icon(xfce_handle *handle);
static void send_workspace(xfce_handle *handle, struct wl_resource *workspace);
static void handle_workspace_destroy(struct wl_listener *listener, void *data);

void xfce_handle_publish(xfce_handle *handle)
{
    if (handle->entry == NULL) {
        return;
    }
    xfce_published wanted = wanted_of(handle);
    const xfce_published *current = &handle->published;
    if (is_published(current, &wanted)) {
        return;
    }
    if (wanted.states != current->states) {
        send_states(handle, wanted.states);
    }
    if (!current->icon_sent || wanted.icon_serial != current->icon_serial) {
        send_icon(handle);
    }
    if (wanted.workspace != current->workspace) {
        send_workspace(handle, wanted.workspace);
    }
    zwlr_foreign_toplevel_handle_v1_send_done(handle->wlr_resource);
}

void xfce_handle_forget_workspace(xfce_handle *handle)
{
    listener_disconnect(&handle->workspace_destroy);
    handle->published.workspace = NULL;
}

static xfce_published wanted_of(const xfce_handle *handle)
{
    const view *v = handle->entry->view;
    const wm_window_view *state = view_state(v);
    return (xfce_published){
        .states = wanted_states(state),
        .icon_serial = icon_slot_serial(view_icon_slot(v)),
        .icon_sent = true,
        .workspace = wanted_workspace(handle, state),
    };
}

static bool is_published(const xfce_published *current, const xfce_published *wanted)
{
    return current->states == wanted->states && current->icon_sent &&
           current->icon_serial == wanted->icon_serial && current->workspace == wanted->workspace;
}

static uint32_t wanted_states(const wm_window_view *state)
{
    return state_bit(XFCE_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_SHADED, state->shaded) |
           state_bit(XFCE_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_SKIP_PAGER, state->skip_pager) |
           state_bit(XFCE_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_SKIP_TASKLIST, state->skip_taskbar) |
           state_bit(XFCE_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_STICKY, state->sticky) |
           state_bit(XFCE_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ABOVE, state->above) |
           state_bit(XFCE_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_BELOW, state->below) |
           state_bit(XFCE_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_DEMANDS_ATTENTION, state->urgent);
}

static uint32_t state_bit(enum xfce_foreign_toplevel_handle_v1_state state, bool enabled)
{
    return enabled ? 1u << state : 0;
}

static struct wl_resource *wanted_workspace(const xfce_handle *handle,
                                            const wm_window_view *state)
{
    const workspace_protocol *workspaces = handle->entry->owner->server->workspaces;
    struct wlr_ext_workspace_handle_v1 *workspace =
        state->sticky ? NULL : workspace_protocol_handle(workspaces, state->workspace_id);
    struct wl_resource *current = handle->published.workspace;
    if (workspace == NULL) {
        return NULL;
    }
    if (current != NULL && ext_workspace_from_resource(current) == workspace) {
        return current;
    }
    return ext_workspace_resource_of_client(wl_resource_get_client(handle->resource), workspace);
}

static void send_states(xfce_handle *handle, uint32_t states)
{
    struct wl_array array;
    wl_array_init(&array);
    for (uint32_t state = 0; state < STATE_COUNT; state++) {
        if ((states & (1u << state)) != 0) {
            append_state(&array, state);
        }
    }
    xfce_foreign_toplevel_handle_v1_send_state(handle->resource, &array);
    wl_array_release(&array);
    handle->published.states = states;
}

static void append_state(struct wl_array *array, uint32_t state)
{
    uint32_t *slot = wl_array_add(array, sizeof(*slot));
    if (slot != NULL) {
        *slot = state;
    }
}

static void send_icon(xfce_handle *handle)
{
    const icon_slot *slot = view_icon_slot(handle->entry->view);
    const char *name = icon_slot_app_name(slot);
    cairo_surface_t *pixels = icon_slot_app_pixels(slot);
    if (name != NULL) {
        xfce_foreign_toplevel_handle_v1_send_icon_name(handle->resource, name);
    }
    if (pixels != NULL) {
        icon_size size = xfce_icon_size_of(pixels);
        xfce_foreign_toplevel_handle_v1_send_icon_size(handle->resource, size.size, size.scale);
    }
    if (name == NULL && pixels == NULL) {
        xfce_foreign_toplevel_handle_v1_send_no_icon(handle->resource);
    }
    handle->published.icon_sent = true;
    handle->published.icon_serial = icon_slot_serial(slot);
}

static void send_workspace(xfce_handle *handle, struct wl_resource *workspace)
{
    if (handle->published.workspace != NULL) {
        xfce_foreign_toplevel_handle_v1_send_workspace_leave(handle->resource,
                                                             handle->published.workspace);
    }
    xfce_handle_forget_workspace(handle);
    if (workspace == NULL) {
        return;
    }
    xfce_foreign_toplevel_handle_v1_send_workspace_enter(handle->resource, workspace);
    handle->published.workspace = workspace;
    handle->workspace_destroy.notify = handle_workspace_destroy;
    wl_resource_add_destroy_listener(workspace, &handle->workspace_destroy);
}

static void handle_workspace_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    xfce_handle *handle = wl_container_of(listener, handle, workspace_destroy);
    xfce_handle_forget_workspace(handle);
}
