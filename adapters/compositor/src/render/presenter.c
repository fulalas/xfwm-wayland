#include "toplevel/foreign_toplevel.h"
#include "input/idle.h"
#include "osd/osd.h"
#include "input/seat.h"
#include "server/server.h"
#include "shell/view.h"
#include "workspace/workspace_protocol.h"
#include "xwayland/xwayland.h"
#include "output/zoom.h"

static void present_window(void *self, void *window, const wm_window_view *state);
static void present_stacking(void *self, void *const *windows, size_t count);
static void present_focus(void *self, void *window);
static void present_workspaces(void *self, const wm_workspaces_view *view);
static void present_cycle(void *self, const wm_cycle_view *view);
static void hide_cycle(void *self);
static void present_menu(void *self, const wm_menu_view *view);
static void hide_menu(void *self);
static void present_outline(void *self, const wm_rect *rect);
static void hide_outline(void *self);
static void present_size(void *self, const wm_size_view *view);
static void hide_size(void *self);
static void present_zoom(void *self, const wm_zoom_view *view);
static void present_cursor(void *self, wm_cursor_shape shape);
static void present_busy(void *self, bool busy);

wm_presenter compositor_presenter(compositor *c)
{
    wm_presenter presenter = {
        .self = c,
        .window_changed = present_window,
        .stacking_changed = present_stacking,
        .focus_changed = present_focus,
        .workspaces_changed = present_workspaces,
        .cycle_shown = present_cycle,
        .cycle_hidden = hide_cycle,
        .menu_shown = present_menu,
        .menu_hidden = hide_menu,
        .outline_shown = present_outline,
        .outline_hidden = hide_outline,
        .size_shown = present_size,
        .size_hidden = hide_size,
        .zoom_changed = present_zoom,
        .cursor_changed = present_cursor,
        .busy_changed = present_busy,
    };
    return presenter;
}

static void present_window(void *self, void *window, const wm_window_view *state)
{
    compositor *c = self;
    view_present(window, state);
    idle_schedule_refresh(c->idle);
    seat_repick_pointer(c->seat);
}

static void present_stacking(void *self, void *const *windows, size_t count)
{
    compositor *c = self;
    views_stack(windows, count);
    idle_schedule_refresh(c->idle);
    seat_repick_pointer(c->seat);
}

static void present_focus(void *self, void *window)
{
    compositor *c = self;
    seat_focus_view(c->seat, window);
}

static void present_workspaces(void *self, const wm_workspaces_view *view)
{
    compositor *c = self;
    workspace_protocol_publish(c->workspaces, view);
    if (c->xwayland != NULL) {
        xwayland_shell_publish_workspaces(c->xwayland, view);
    }
    if (c->foreign_toplevels != NULL) {
        foreign_toplevels_sync_all(c->foreign_toplevels);
    }
}

static void present_cycle(void *self, const wm_cycle_view *view)
{
    compositor *c = self;
    osd_show_cycle(c->osd, view);
    seat_repick_pointer(c->seat);
}

static void hide_cycle(void *self)
{
    compositor *c = self;
    osd_hide_cycle(c->osd);
    seat_repick_pointer(c->seat);
    seat_focus_refresh(c->seat);
}

static void present_menu(void *self, const wm_menu_view *view)
{
    compositor *c = self;
    osd_show_menu(c->osd, view);
    seat_repick_pointer(c->seat);
}

static void hide_menu(void *self)
{
    compositor *c = self;
    osd_hide_menu(c->osd);
    seat_repick_pointer(c->seat);
    seat_focus_refresh(c->seat);
}

static void present_outline(void *self, const wm_rect *rect)
{
    compositor *c = self;
    osd_show_outline(c->osd, rect);
}

static void hide_outline(void *self)
{
    compositor *c = self;
    osd_hide_outline(c->osd);
}

static void present_size(void *self, const wm_size_view *view)
{
    compositor *c = self;
    osd_show_size(c->osd, view);
}

static void hide_size(void *self)
{
    compositor *c = self;
    osd_hide_size(c->osd);
}

static void present_zoom(void *self, const wm_zoom_view *view)
{
    compositor *c = self;
    zoom_set(c->zoom, view);
}

static void present_cursor(void *self, wm_cursor_shape shape)
{
    compositor *c = self;
    seat_set_core_cursor(c->seat, shape);
}

static void present_busy(void *self, bool busy)
{
    compositor *c = self;
    seat_set_busy(c->seat, busy);
}
