#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_ext_workspace_v1.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/util/log.h>

#include "util/listener.h"
#include "output/output.h"
#include "workspace_protocol.h"

enum {
    WORKSPACE_TEXT_MAX = 64,
    COORDINATE_COUNT = 2,
};

static const uint32_t WORKSPACE_CAPS = EXT_WORKSPACE_HANDLE_V1_WORKSPACE_CAPABILITIES_ACTIVATE |
                                       EXT_WORKSPACE_HANDLE_V1_WORKSPACE_CAPABILITIES_REMOVE;
static const uint32_t GROUP_CAPS =
    EXT_WORKSPACE_GROUP_HANDLE_V1_GROUP_CAPABILITIES_CREATE_WORKSPACE;

struct workspace_protocol {
    compositor *server;
    struct wlr_ext_workspace_group_handle_v1 *group;
    struct wlr_ext_workspace_handle_v1 *handles[WM_WORKSPACE_MAX];
    unsigned ids[WM_WORKSPACE_MAX];
    int count;
    struct wl_listener commit;
    struct wl_listener layout_change;
};

static void handle_commit(struct wl_listener *listener, void *data);
static void handle_layout_change(struct wl_listener *listener, void *data);
static void apply_request(workspace_protocol *workspaces,
                          const struct wlr_ext_workspace_v1_request *request);
static void switch_to(workspace_protocol *workspaces, int index);
static void remove_at(workspace_protocol *workspaces, int index);
static int index_of(const workspace_protocol *workspaces,
                    const struct wlr_ext_workspace_handle_v1 *handle);
static void destroy_all(workspace_protocol *workspaces);
static void reconcile(workspace_protocol *workspaces, const wm_workspaces_view *view);
static struct wlr_ext_workspace_handle_v1 *take_handle(workspace_protocol *workspaces, unsigned id);
static struct wlr_ext_workspace_handle_v1 *create_handle(workspace_protocol *workspaces,
                                                         unsigned id);
static void set_position(struct wlr_ext_workspace_handle_v1 *handle, int index, int columns);
static void update_handle(workspace_protocol *workspaces, int index,
                          const wm_workspaces_view *view);

workspace_protocol *workspace_protocol_create(compositor *server)
{
    workspace_protocol *w = calloc(1, sizeof(*w));
    if (w == NULL) {
        return NULL;
    }
    w->server = server;
    wl_list_init(&w->commit.link);
    struct wlr_ext_workspace_manager_v1 *manager = server->globals.workspace;
    w->group = wlr_ext_workspace_group_handle_v1_create(manager, GROUP_CAPS);
    if (w->group != NULL) {
        listener_connect(&w->commit, &manager->events.commit, handle_commit);
    }
    listener_connect(&w->layout_change, &server->layout->events.change, handle_layout_change);
    return w;
}

void workspace_protocol_destroy(workspace_protocol *w)
{
    destroy_all(w);
    if (w->group != NULL) {
        wlr_ext_workspace_group_handle_v1_destroy(w->group);
    }
    listener_disconnect(&w->commit);
    listener_disconnect(&w->layout_change);
    free(w);
}

void workspace_protocol_publish(workspace_protocol *w, const wm_workspaces_view *view)
{
    if (w == NULL || w->group == NULL) {
        return;
    }
    reconcile(w, view);
    for (int i = 0; i < w->count; i++) {
        update_handle(w, i, view);
    }
}

struct wlr_ext_workspace_handle_v1 *workspace_protocol_handle(const workspace_protocol *w,
                                                              unsigned id)
{
    for (int i = 0; w != NULL && i < w->count; i++) {
        if (w->ids[i] == id) {
            return w->handles[i];
        }
    }
    return NULL;
}

int workspace_protocol_index(const workspace_protocol *w,
                             const struct wlr_ext_workspace_handle_v1 *handle)
{
    return w != NULL ? index_of(w, handle) : -1;
}

static void handle_commit(struct wl_listener *listener, void *data)
{
    workspace_protocol *w = wl_container_of(listener, w, commit);
    const struct wlr_ext_workspace_v1_commit_event *event = data;
    const struct wlr_ext_workspace_v1_request *request;
    wl_list_for_each(request, event->requests, link) {
        apply_request(w, request);
    }
}

static void handle_layout_change(struct wl_listener *listener, void *data)
{
    (void) data;
    workspace_protocol *w = wl_container_of(listener, w, layout_change);
    outputs *outs = w->server->outputs;
    if (w->group == NULL || outs == NULL) {
        return;
    }
    for (size_t i = 0; i < outputs_known_count(outs); i++) {
        output *o = outputs_known_at(outs, i);
        if (output_is_active(o)) {
            wlr_ext_workspace_group_handle_v1_output_enter(w->group, output_wlr(o));
        } else {
            wlr_ext_workspace_group_handle_v1_output_leave(w->group, output_wlr(o));
        }
    }
}

static void apply_request(workspace_protocol *w,
                          const struct wlr_ext_workspace_v1_request *request)
{
    wm_desktop *desktop = w->server->desktop;
    switch (request->type) {
    case WLR_EXT_WORKSPACE_V1_REQUEST_CREATE_WORKSPACE:
        wm_desktop_add_workspace(desktop);
        break;
    case WLR_EXT_WORKSPACE_V1_REQUEST_ACTIVATE:
        switch_to(w, index_of(w, request->activate.workspace));
        break;
    case WLR_EXT_WORKSPACE_V1_REQUEST_REMOVE:
        remove_at(w, index_of(w, request->remove.workspace));
        break;
    default:
        break;
    }
}

static void switch_to(workspace_protocol *w, int index)
{
    if (index >= 0) {
        wm_desktop_switch_workspace(w->server->desktop, index);
    }
}

static void remove_at(workspace_protocol *w, int index)
{
    if (index >= 0) {
        wm_desktop_remove_workspace(w->server->desktop, index);
    }
}

static int index_of(const workspace_protocol *w, const struct wlr_ext_workspace_handle_v1 *handle)
{
    for (int i = 0; handle != NULL && i < w->count; i++) {
        if (w->handles[i] == handle) {
            return i;
        }
    }
    return -1;
}

static void destroy_all(workspace_protocol *w)
{
    for (int i = 0; i < w->count; i++) {
        if (w->handles[i] != NULL) {
            wlr_ext_workspace_handle_v1_destroy(w->handles[i]);
        }
    }
    w->count = 0;
}

static void reconcile(workspace_protocol *w, const wm_workspaces_view *view)
{
    struct wlr_ext_workspace_handle_v1 *handles[WM_WORKSPACE_MAX] = {0};
    int count = view->count < WM_WORKSPACE_MAX ? view->count : WM_WORKSPACE_MAX;
    for (int i = 0; i < count; i++) {
        handles[i] = take_handle(w, view->ids[i]);
        if (handles[i] == NULL) {
            handles[i] = create_handle(w, view->ids[i]);
        }
        if (handles[i] != NULL) {
            set_position(handles[i], i, view->columns);
        }
    }
    destroy_all(w);
    memcpy(w->handles, handles, sizeof(handles));
    memcpy(w->ids, view->ids, sizeof(w->ids));
    w->count = count;
}

static struct wlr_ext_workspace_handle_v1 *take_handle(workspace_protocol *w, unsigned id)
{
    for (int i = 0; i < w->count; i++) {
        struct wlr_ext_workspace_handle_v1 *handle = w->handles[i];
        if (handle != NULL && w->ids[i] == id) {
            w->handles[i] = NULL;
            return handle;
        }
    }
    return NULL;
}

static struct wlr_ext_workspace_handle_v1 *create_handle(workspace_protocol *w, unsigned id)
{
    char name[WORKSPACE_TEXT_MAX];
    snprintf(name, sizeof(name), "xfwm-workspace-%u", id);
    struct wlr_ext_workspace_handle_v1 *handle =
        wlr_ext_workspace_handle_v1_create(w->group->manager, name, WORKSPACE_CAPS);
    if (handle == NULL) {
        wlr_log(WLR_ERROR, "cannot create a workspace handle");
        return NULL;
    }
    wlr_ext_workspace_handle_v1_set_group(handle, w->group);
    return handle;
}

static void set_position(struct wlr_ext_workspace_handle_v1 *handle, int index, int columns)
{
    uint32_t coordinates[COORDINATE_COUNT] = {(uint32_t) (index % columns),
                                              (uint32_t) (index / columns)};
    if (handle->coordinates.size == sizeof(coordinates) &&
        memcmp(handle->coordinates.data, coordinates, sizeof(coordinates)) == 0) {
        return;
    }
    wlr_ext_workspace_handle_v1_set_coordinates(handle, coordinates, COORDINATE_COUNT);
}

static void update_handle(workspace_protocol *w, int index, const wm_workspaces_view *view)
{
    struct wlr_ext_workspace_handle_v1 *handle = w->handles[index];
    if (handle == NULL) {
        return;
    }
    char number[WORKSPACE_TEXT_MAX];
    snprintf(number, sizeof(number), "%d", index + 1);
    const char *name = view->names[index] != NULL ? view->names[index] : number;
    if (handle->name == NULL || strcmp(handle->name, name) != 0) {
        wlr_ext_workspace_handle_v1_set_name(handle, name);
    }
    bool active = index == view->current;
    bool was_active = (handle->state & EXT_WORKSPACE_HANDLE_V1_STATE_ACTIVE) != 0;
    if (active != was_active) {
        wlr_ext_workspace_handle_v1_set_active(handle, active);
    }
}
