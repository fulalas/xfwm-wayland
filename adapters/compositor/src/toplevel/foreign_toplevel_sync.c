#include <string.h>
#include <wlr/types/wlr_ext_foreign_toplevel_list_v1.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>

#include "foreign_toplevel_private.h"
#include "output/output.h"
#include "xfce_toplevel.h"

typedef struct state_flag {
    uint32_t bit;
    void (*apply)(struct wlr_foreign_toplevel_handle_v1 *handle, bool value);
} state_flag;

static const state_flag state_flags[] = {
    {WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED, wlr_foreign_toplevel_handle_v1_set_maximized},
    {WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED, wlr_foreign_toplevel_handle_v1_set_minimized},
    {WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED, wlr_foreign_toplevel_handle_v1_set_activated},
    {WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN,
     wlr_foreign_toplevel_handle_v1_set_fullscreen},
};

static bool ensure_ext(toplevel_entry *entry);
static bool ensure_wlr(toplevel_entry *entry);
static void hide_wlr(toplevel_entry *entry);
static void sync_ext_identity(toplevel_entry *entry);
static void sync_wlr_identity(toplevel_entry *entry);
static void sync_states(toplevel_entry *entry);
static void leave_outputs(toplevel_entry *entry);
static void enter_outputs(toplevel_entry *entry);
static void sync_parent(toplevel_entry *entry);
static uint32_t wanted_states(const wm_window_view *state);
static bool shown_on(const toplevel_entry *entry, const output *output);
static bool has_output(const struct wlr_foreign_toplevel_handle_v1 *handle,
                       const struct wlr_output *output);
static bool same_text(const char *current, const char *wanted);

void toplevel_entry_sync(toplevel_entry *entry)
{
    if (ensure_ext(entry)) {
        sync_ext_identity(entry);
    }
    const wm_window_view *state = view_state(entry->view);
    if (state->skip_taskbar && state->parent == NULL) {
        hide_wlr(entry);
        return;
    }
    if (!ensure_wlr(entry)) {
        return;
    }
    sync_wlr_identity(entry);
    sync_states(entry);
    leave_outputs(entry);
    enter_outputs(entry);
    sync_parent(entry);
    xfce_toplevels_publish(entry);
}

void toplevel_entry_hide(toplevel_entry *entry)
{
    hide_wlr(entry);
    if (entry->ext != NULL) {
        wlr_ext_foreign_toplevel_handle_v1_destroy(entry->ext);
        entry->ext = NULL;
    }
}

static void hide_wlr(toplevel_entry *entry)
{
    xfce_toplevels_detach(entry);
    toplevel_requests_disconnect(entry);
    if (entry->wlr != NULL) {
        wlr_foreign_toplevel_handle_v1_destroy(entry->wlr);
        entry->wlr = NULL;
    }
}

static bool ensure_wlr(toplevel_entry *entry)
{
    if (entry->wlr == NULL) {
        entry->wlr =
            wlr_foreign_toplevel_handle_v1_create(entry->owner->server->globals.foreign_toplevel);
        if (entry->wlr != NULL) {
            entry->wlr->data = entry;
            toplevel_requests_connect(entry);
        }
    }
    return entry->wlr != NULL;
}

static bool ensure_ext(toplevel_entry *entry)
{
    protocol_globals *globals = &entry->owner->server->globals;
    if (entry->ext == NULL) {
        struct wlr_ext_foreign_toplevel_handle_v1_state state = {
            .title = view_state(entry->view)->title,
            .app_id = view_state(entry->view)->app_id,
        };
        entry->ext = wlr_ext_foreign_toplevel_handle_v1_create(globals->foreign_toplevel_list,
                                                               &state);
        if (entry->ext != NULL) {
            entry->ext->data = entry;
        }
    }
    return entry->ext != NULL;
}

static void sync_ext_identity(toplevel_entry *entry)
{
    const char *title = view_state(entry->view)->title;
    const char *app_id = view_state(entry->view)->app_id;
    if (!same_text(entry->ext->title, title) || !same_text(entry->ext->app_id, app_id)) {
        struct wlr_ext_foreign_toplevel_handle_v1_state state = {title, app_id};
        wlr_ext_foreign_toplevel_handle_v1_update_state(entry->ext, &state);
    }
}

static void sync_wlr_identity(toplevel_entry *entry)
{
    const char *title = view_state(entry->view)->title;
    const char *app_id = view_state(entry->view)->app_id;
    if (!same_text(entry->wlr->title, title)) {
        wlr_foreign_toplevel_handle_v1_set_title(entry->wlr, title);
    }
    if (!same_text(entry->wlr->app_id, app_id)) {
        wlr_foreign_toplevel_handle_v1_set_app_id(entry->wlr, app_id);
    }
}

static void sync_states(toplevel_entry *entry)
{
    uint32_t wanted = wanted_states(view_state(entry->view));
    uint32_t changed = wanted ^ entry->wlr->state;
    for (size_t i = 0; i < sizeof(state_flags) / sizeof(state_flags[0]); i++) {
        if ((changed & state_flags[i].bit) != 0) {
            state_flags[i].apply(entry->wlr, (wanted & state_flags[i].bit) != 0);
        }
    }
}

static void leave_outputs(toplevel_entry *entry)
{
    struct wlr_foreign_toplevel_handle_v1_output *current;
    struct wlr_foreign_toplevel_handle_v1_output *tmp;
    wl_list_for_each_safe(current, tmp, &entry->wlr->outputs, link) {
        if (!shown_on(entry, output_from_wlr(current->output))) {
            wlr_foreign_toplevel_handle_v1_output_leave(entry->wlr, current->output);
        }
    }
}

static void enter_outputs(toplevel_entry *entry)
{
    outputs *outs = entry->owner->server->outputs;
    for (output *o = outputs_next_active(outs, NULL); o != NULL; o = outputs_next_active(outs, o)) {
        if (shown_on(entry, o) && !has_output(entry->wlr, output_wlr(o))) {
            wlr_foreign_toplevel_handle_v1_output_enter(entry->wlr, output_wlr(o));
        }
    }
}

static void sync_parent(toplevel_entry *entry)
{
    view *parent = view_state(entry->view)->parent;
    toplevel_entry *parent_entry =
        parent != NULL ? foreign_toplevels_find(entry->owner, parent) : NULL;
    struct wlr_foreign_toplevel_handle_v1 *handle =
        parent_entry != NULL ? parent_entry->wlr : NULL;
    if (entry->wlr->parent != handle) {
        wlr_foreign_toplevel_handle_v1_set_parent(entry->wlr, handle);
    }
}

static uint32_t wanted_states(const wm_window_view *state)
{
    uint32_t states = 0;
    states |= state->maximized ? WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED : 0;
    states |= state->minimized ? WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED : 0;
    states |= state->activated ? WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED : 0;
    states |= state->fullscreen ? WLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN : 0;
    return states;
}

static bool shown_on(const toplevel_entry *entry, const output *o)
{
    if (o == NULL) {
        return false;
    }
    wm_rect frame = view_state(entry->view)->frame;
    wm_rect area = output_full(o);
    return wm_rect_intersects(&frame, &area);
}

static bool has_output(const struct wlr_foreign_toplevel_handle_v1 *handle,
                       const struct wlr_output *output)
{
    const struct wlr_foreign_toplevel_handle_v1_output *current;
    wl_list_for_each(current, &handle->outputs, link) {
        if (current->output == output) {
            return true;
        }
    }
    return false;
}

static bool same_text(const char *current, const char *wanted)
{
    return current != NULL && strcmp(current, wanted) == 0;
}
