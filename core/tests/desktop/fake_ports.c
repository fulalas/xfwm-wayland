#include "fake_ports.h"

#include <string.h>

static char handles[FAKE_WINDOW_MAX];

static fake_world *world_of(void *self);
static void window_changed(void *self, void *window, const wm_window_view *view);
static void stacking_changed(void *self, void *const *windows, size_t count);
static void focus_changed(void *self, void *window);
static void workspaces_changed(void *self, const wm_workspaces_view *view);
static void cycle_shown(void *self, const wm_cycle_view *view);
static void cycle_hidden(void *self);
static void menu_shown(void *self, const wm_menu_view *view);
static void menu_hidden(void *self);
static void outline_shown(void *self, const wm_rect *rect);
static void outline_hidden(void *self);
static void size_shown(void *self, const wm_size_view *view);
static void size_hidden(void *self);
static void busy_changed(void *self, bool busy);
static void zoom_changed(void *self, const wm_zoom_view *view);
static void cursor_changed(void *self, wm_cursor_shape shape);
static void configure(void *self, void *window, const wm_rect *client);
static void close_window(void *self, void *window);
static void ask_terminate(void *self, void *window);
static void warp_pointer(void *self, wm_point point);
static void spawn(void *self, const char *command);
static void app_shortcut(void *self, uint32_t id, bool pressed, uint32_t time_ms);
static void logout(void *self);
static void store_workspace_count(void *self, int count);
static void store_workspace_names(void *self, const char *const *names, size_t count);
static void store_workspace_rows(void *self, int rows);
static void schedule_timer(void *self, uint32_t delay_ms);
static size_t output_count(void *self);
static wm_output_info output_at_index(void *self, size_t index);
static wm_point pointer_position(void *self);
static uint32_t now_ms(void *self);
static wm_size measure_title(void *self, const char *text);
static bool menu_item_matches_key(void *self, const wm_menu_level *level, size_t index,
                                  uint32_t keysym);
static void set_all_states(wm_size sizes[WM_FRAME_STATE_COUNT], wm_size size);

void fake_world_init(fake_world *world)
{
    memset(world, 0, sizeof(*world));
    world->ports.presenter = (wm_presenter) {
        world, window_changed, stacking_changed, focus_changed, workspaces_changed,
        cycle_shown, cycle_hidden, menu_shown, menu_hidden, outline_shown, outline_hidden,
        size_shown, size_hidden, zoom_changed, cursor_changed, busy_changed,
    };
    world->ports.commands = (wm_commands) {
        world, configure, close_window, ask_terminate, warp_pointer, spawn, app_shortcut,
        logout, store_workspace_count, store_workspace_names, store_workspace_rows,
        schedule_timer,
    };
    world->ports.environment = (wm_environment) {
        world, output_count, output_at_index, pointer_position, now_ms, measure_title,
        menu_item_matches_key,
    };
    world->now_ms = 1000;
    wm_rect screen = {0, 0, FAKE_SCREEN_WIDTH, FAKE_SCREEN_HEIGHT};
    fake_world_add_output(world, screen, screen);
}

void fake_world_add_output(fake_world *world, wm_rect full, wm_rect usable)
{
    world->outputs[world->output_count++] = (wm_output_info) {full, usable, false};
}

wm_config fake_config(void)
{
    wm_config config = wm_config_defaults();
    config.workspaces.count = 4;
    return config;
}

wm_theme_metrics fake_theme(void)
{
    wm_theme_metrics theme;
    memset(&theme, 0, sizeof(theme));
    set_all_states(theme.side[WM_SIDE_LEFT], (wm_size) {FAKE_SIDE, 1});
    set_all_states(theme.side[WM_SIDE_RIGHT], (wm_size) {FAKE_SIDE, 1});
    set_all_states(theme.side[WM_SIDE_BOTTOM], (wm_size) {1, FAKE_SIDE});
    set_all_states(theme.corner[WM_CORNER_TOP_LEFT], (wm_size) {FAKE_CORNER, FAKE_TITLE});
    set_all_states(theme.corner[WM_CORNER_TOP_RIGHT], (wm_size) {FAKE_CORNER, FAKE_TITLE});
    set_all_states(theme.corner[WM_CORNER_BOTTOM_LEFT], (wm_size) {FAKE_CORNER, FAKE_CORNER});
    set_all_states(theme.corner[WM_CORNER_BOTTOM_RIGHT], (wm_size) {FAKE_CORNER, FAKE_CORNER});
    for (int part = 0; part < WM_TITLE_PART_COUNT; part++) {
        set_all_states(theme.title[part], (wm_size) {4, FAKE_TITLE});
    }
    for (int button = 0; button < WM_BUTTON_COUNT; button++) {
        theme.button[button][WM_IMAGE_ACTIVE] = (wm_size) {FAKE_BUTTON, FAKE_BUTTON};
        theme.button[button][WM_IMAGE_INACTIVE] = (wm_size) {FAKE_BUTTON, FAKE_BUTTON};
    }
    return theme;
}

wm_desktop *fake_desktop_create(fake_world *world, const wm_config *config)
{
    wm_desktop *desktop = wm_desktop_create(&world->ports, config);
    wm_theme_metrics theme = fake_theme();
    wm_desktop_set_appearance(desktop, &theme, config);
    return desktop;
}

void *fake_handle(int id)
{
    return &handles[id];
}

wm_window_desc fake_desc(int id, wm_rect geometry)
{
    wm_window_desc desc;
    memset(&desc, 0, sizeof(desc));
    desc.handle = fake_handle(id);
    desc.type = WM_TYPE_NORMAL;
    desc.capabilities = WM_CAP_ALL;
    desc.geometry = geometry;
    desc.title = "window";
    desc.app_id = "app";
    desc.opacity = 1.0;
    desc.activation_allowed = true;
    return desc;
}

wm_window *fake_map(wm_desktop *desktop, int id, wm_rect geometry)
{
    wm_window_desc desc = fake_desc(id, geometry);
    return wm_window_map(desktop, &desc);
}

wm_window_desc fake_dialog_desc(int id, wm_window *parent)
{
    wm_window_desc desc = fake_desc(id, (wm_rect) {0, 0, 200, 100});
    desc.type = WM_TYPE_DIALOG;
    desc.parent = parent;
    return desc;
}

wm_window *fake_map_dialog(wm_desktop *desktop, int id, wm_window *parent)
{
    wm_window_desc desc = fake_dialog_desc(id, parent);
    return wm_window_map(desktop, &desc);
}

wm_rect fake_client_of(const wm_window *window)
{
    return wm_window_current_view(window).client;
}

wm_rect fake_configured(const fake_world *world, void *handle)
{
    for (size_t i = 0; i < world->configure_count; i++) {
        if (world->configures[i].window == handle) {
            return world->configures[i].rect;
        }
    }
    return (wm_rect) {0, 0, 0, 0};
}

size_t fake_stack_index(const fake_world *world, void *handle)
{
    for (size_t i = 0; i < world->stacking_count; i++) {
        if (world->stacking[i] == handle) {
            return i;
        }
    }
    return FAKE_WINDOW_MAX;
}

void fake_advance(fake_world *world, wm_desktop *desktop, uint32_t ms)
{
    uint32_t target = world->now_ms + ms;
    while (world->timer_pending && (int32_t) (world->timer_deadline_ms - target) <= 0) {
        world->now_ms = world->timer_deadline_ms;
        world->timer_pending = false;
        wm_desktop_handle_timer(desktop);
    }
    world->now_ms = target;
}

static fake_world *world_of(void *self)
{
    return (fake_world *) self;
}

static void window_changed(void *self, void *window, const wm_window_view *view)
{
    (void) window;
    (void) view;
    world_of(self)->window_changed_calls++;
}

static void stacking_changed(void *self, void *const *windows, size_t count)
{
    fake_world *world = world_of(self);
    world->stacking_calls++;
    world->stacking_count = count < FAKE_WINDOW_MAX ? count : FAKE_WINDOW_MAX;
    for (size_t i = 0; i < world->stacking_count; i++) {
        world->stacking[i] = windows[i];
    }
}

static void focus_changed(void *self, void *window)
{
    world_of(self)->focused = window;
    world_of(self)->focus_calls++;
}

static void workspaces_changed(void *self, const wm_workspaces_view *view)
{
    world_of(self)->workspaces = *view;
    world_of(self)->workspaces_calls++;
}

static void cycle_shown(void *self, const wm_cycle_view *view)
{
    world_of(self)->cycle = *view;
    world_of(self)->cycle_visible = true;
}

static void cycle_hidden(void *self)
{
    world_of(self)->cycle_visible = false;
}

static void menu_shown(void *self, const wm_menu_view *view)
{
    world_of(self)->menu = *view;
    world_of(self)->menu_visible = true;
}

static void menu_hidden(void *self)
{
    world_of(self)->menu_visible = false;
}

static void outline_shown(void *self, const wm_rect *rect)
{
    world_of(self)->outline = *rect;
    world_of(self)->outline_visible = true;
}

static void outline_hidden(void *self)
{
    world_of(self)->outline_visible = false;
}

static void size_shown(void *self, const wm_size_view *view)
{
    world_of(self)->size = *view;
    world_of(self)->size_visible = true;
}

static void size_hidden(void *self)
{
    world_of(self)->size_visible = false;
}

static void busy_changed(void *self, bool busy)
{
    world_of(self)->busy = busy;
}

static void zoom_changed(void *self, const wm_zoom_view *view)
{
    world_of(self)->zoom = *view;
    world_of(self)->zoom_calls++;
}

static void cursor_changed(void *self, wm_cursor_shape shape)
{
    world_of(self)->cursor = shape;
}

static void configure(void *self, void *window, const wm_rect *client)
{
    fake_world *world = world_of(self);
    world->configure_calls++;
    for (size_t i = 0; i < world->configure_count; i++) {
        if (world->configures[i].window == window) {
            world->configures[i].rect = *client;
            return;
        }
    }
    if (world->configure_count < FAKE_WINDOW_MAX) {
        world->configures[world->configure_count++] = (fake_configure) {window, *client};
    }
}

static void close_window(void *self, void *window)
{
    world_of(self)->closed = window;
}

static void ask_terminate(void *self, void *window)
{
    world_of(self)->terminate_asked = window;
}

static void warp_pointer(void *self, wm_point point)
{
    world_of(self)->warp = point;
    world_of(self)->pointer = point;
    world_of(self)->warp_calls++;
}

static void spawn(void *self, const char *command)
{
    fake_world *world = world_of(self);
    strncpy(world->spawned, command, FAKE_TEXT_MAX - 1);
}

static void app_shortcut(void *self, uint32_t id, bool pressed, uint32_t time_ms)
{
    (void) time_ms;
    fake_world *world = world_of(self);
    world->app_shortcut_id = id;
    if (pressed) {
        world->app_shortcut_presses++;
    } else {
        world->app_shortcut_releases++;
    }
}

static void logout(void *self)
{
    world_of(self)->logout_calls++;
}

static void store_workspace_count(void *self, int count)
{
    world_of(self)->stored_workspace_count = count;
    world_of(self)->store_calls++;
}

static void store_workspace_names(void *self, const char *const *names, size_t count)
{
    (void) names;
    world_of(self)->stored_name_count = (int) count;
}

static void store_workspace_rows(void *self, int rows)
{
    world_of(self)->stored_rows = rows;
}

static void schedule_timer(void *self, uint32_t delay_ms)
{
    fake_world *world = world_of(self);
    world->timer_pending = true;
    world->timer_deadline_ms = world->now_ms + delay_ms;
}

static size_t output_count(void *self)
{
    return world_of(self)->output_count;
}

static wm_output_info output_at_index(void *self, size_t index)
{
    return world_of(self)->outputs[index];
}

static wm_point pointer_position(void *self)
{
    return world_of(self)->pointer;
}

static uint32_t now_ms(void *self)
{
    return world_of(self)->now_ms;
}

static wm_size measure_title(void *self, const char *text)
{
    (void) self;
    return (wm_size) {(int) strlen(text) * 6, 10};
}

static bool menu_item_matches_key(void *self, const wm_menu_level *level, size_t index,
                                  uint32_t keysym)
{
    (void) self;
    return level->items[index].label == WM_MENU_CLOSE && keysym == 'c';
}

static void set_all_states(wm_size sizes[WM_FRAME_STATE_COUNT], wm_size size)
{
    sizes[WM_FRAME_ACTIVE] = size;
    sizes[WM_FRAME_INACTIVE] = size;
}
