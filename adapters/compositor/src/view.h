#ifndef COMPOSITOR_VIEW_H
#define COMPOSITOR_VIEW_H

#include <cairo.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include "server.h"
#include "wm/desktop.h"

struct wlr_scene_tree;
struct wlr_surface;

typedef struct view view;

typedef struct view_impl {
    struct wlr_surface *(*surface)(void *self);
    void (*configure)(void *self, const wm_rect *client);
    void (*close)(void *self);
    pid_t (*pid)(void *self);
    void (*send_states)(void *self, const wm_window_view *state);
    void (*focus)(void *self);
    void (*unfocus)(void *self);
    cairo_surface_t *(*icon)(void *self);
} view_impl;

view *view_create(compositor *server, const view_impl *impl, void *self);
void view_destroy(view *view);
void view_map(view *view, const wm_window_desc *desc);

struct wlr_scene_tree *view_content_tree(const view *view);
struct wlr_scene_tree *view_popup_tree(const view *view);
wm_window *view_window(const view *view);
struct wlr_surface *view_surface(const view *view);
cairo_surface_t *view_icon(const view *view);
const char *view_title(const view *view);
wm_rect view_client_rect(const view *view);
wm_rect view_output_area(const view *view);
bool view_is_fullscreen(const view *view);
pid_t view_pid(const view *view);
const wm_window_view *view_state(const view *view);
view *views_find_by_surface(compositor *server, struct wlr_surface *surface);

void view_configure(view *view, const wm_rect *client);
void view_close(view *view);
void view_kill(view *view);
void kill_client_pid(pid_t pid);
void view_focus(view *view);
void view_unfocus(view *view);
void views_stack(void *const *windows, size_t count);
void view_present(view *view, const wm_window_view *state);
void view_committed(view *view, wm_size size);
void view_configure_settled(view *view);
void view_request_state(view *view, const wm_state_change *change);
wm_state_change view_state_change(uint32_t states, bool enabled);
void view_icon_changed(view *view);
void view_report_not_responding(view *view);
void views_invalidate_decorations(compositor *server);

#endif
