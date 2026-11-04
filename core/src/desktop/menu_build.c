#include "menu.h"

#include "desktop_state.h"
#include "monitor_move.h"

typedef struct menu_builder {
    const wm_desktop *desktop;
    const wm_window *window;
    wm_menu_level *level;
} menu_builder;

static void add(menu_builder *builder, wm_menu_label label, bool sensitive);
static void add_item(menu_builder *builder, wm_menu_item item);
static void add_separator(menu_builder *builder);
static void build_window_ops(menu_builder *builder);
static void build_layers(menu_builder *builder);
static void build_states(menu_builder *builder);
static void build_placement(menu_builder *builder);
static bool has_monitor_targets(const wm_desktop *desktop, const wm_window *window);
static bool is_transient(const wm_window *window);
static wm_menu_mark radio(bool on);
static void build_workspaces(const wm_desktop *desktop, const wm_window *window,
                             wm_menu_level *level);
static void build_monitors(const wm_desktop *desktop, const wm_window *window,
                           wm_menu_level *level);

void menu_build(const wm_desktop *desktop, const wm_window *window, wm_menu_level *level)
{
    menu_builder builder = {desktop, window, level};
    level->count = 0;
    build_window_ops(&builder);
    add_separator(&builder);
    build_layers(&builder);
    add_separator(&builder);
    build_states(&builder);
    add_separator(&builder);
    build_placement(&builder);
    add_separator(&builder);
    add(&builder, WM_MENU_CLOSE, window_has_flag(window, WINDOW_HAS_CLOSE));
}

void menu_build_submenu(const wm_desktop *desktop, wm_menu_label label, wm_menu_level *level)
{
    const wm_window *window = desktop->menu.window;
    level->count = 0;
    level->selected = -1;
    level->anchor = desktop->menu.view.main.anchor;
    if (label == WM_MENU_WORKSPACES) {
        build_workspaces(desktop, window, level);
    } else {
        build_monitors(desktop, window, level);
    }
}

static void add(menu_builder *builder, wm_menu_label label, bool sensitive)
{
    add_item(builder, (wm_menu_item) {label, NULL, sensitive, false, WM_MENU_MARK_NONE});
}

static void add_item(menu_builder *builder, wm_menu_item item)
{
    wm_menu_level *level = builder->level;
    if (level->count >= WM_MENU_ITEM_MAX) {
        return;
    }
    item.has_submenu = item.label == WM_MENU_WORKSPACES || item.label == WM_MENU_MONITORS;
    level->items[level->count++] = item;
}

static void add_separator(menu_builder *builder)
{
    add(builder, WM_MENU_SEPARATOR, false);
}

static void build_window_ops(menu_builder *builder)
{
    const wm_window *window = builder->window;
    bool fullscreen = window_is_fullscreen(window);
    if (window_has_state(window, WM_STATE_MAXIMIZED)) {
        add(builder, WM_MENU_UNMAXIMIZE, !fullscreen);
    } else {
        add(builder, WM_MENU_MAXIMIZE, !fullscreen && window_can_maximize(window));
    }
    if (window_is_minimized(window)) {
        add(builder, WM_MENU_MINIMIZE_OTHERS, true);
        add(builder, WM_MENU_SHOW, true);
    } else {
        add(builder, WM_MENU_MINIMIZE, window_can_hide(window));
        add(builder, WM_MENU_MINIMIZE_OTHERS, true);
    }
    add(builder, WM_MENU_MOVE, window_has_flag(window, WINDOW_HAS_MOVE) && !fullscreen);
    bool resizable = window_can_resize(window) && !window_is_maximized(window);
    add(builder, WM_MENU_RESIZE, resizable && !fullscreen);
}

static void build_layers(menu_builder *builder)
{
    const wm_window *window = builder->window;
    bool sensitive = !is_transient(window) && window_is_regular_focusable(window)
                     && !window_is_fullscreen(window);
    bool above = window_has_state(window, WM_STATE_ABOVE);
    bool below = window_has_state(window, WM_STATE_BELOW);
    add_item(builder, (wm_menu_item) {WM_MENU_ABOVE, NULL, sensitive, false, radio(above)});
    add_item(builder, (wm_menu_item) {WM_MENU_NORMAL, NULL, sensitive, false,
                                      radio(!above && !below)});
    add_item(builder, (wm_menu_item) {WM_MENU_BELOW, NULL, sensitive, false, radio(below)});
}

static void build_states(menu_builder *builder)
{
    const wm_window *window = builder->window;
    bool border = window_has_flag(window, WINDOW_HAS_BORDER);
    if (window_is_shaded(window)) {
        add(builder, WM_MENU_UNSHADE, border);
    } else {
        add(builder, WM_MENU_SHADE, border && !window_is_fullscreen(window));
    }
    bool fullscreen_ok = !is_transient(window) && window->type == WM_TYPE_NORMAL;
    if (window_is_fullscreen(window)) {
        add(builder, WM_MENU_UNFULLSCREEN, fullscreen_ok);
    } else {
        add(builder, WM_MENU_FULLSCREEN, fullscreen_ok);
    }
}

static void build_placement(menu_builder *builder)
{
    const wm_window *window = builder->window;
    bool stick = !is_transient(window) && window_has_flag(window, WINDOW_HAS_STICK);
    bool sticky = window_is_sticky(window);
    wm_menu_mark mark = sticky ? WM_MENU_MARK_CHECK_ON : WM_MENU_MARK_CHECK_OFF;
    add_item(builder, (wm_menu_item) {WM_MENU_STICK, NULL, stick, false, mark});
    add(builder, WM_MENU_WORKSPACES, stick && !sticky);
    if (has_monitor_targets(builder->desktop, window)) {
        add(builder, WM_MENU_MONITORS, true);
    }
}

static bool has_monitor_targets(const wm_desktop *desktop, const wm_window *window)
{
    for (int direction = MONITOR_LEFT; direction <= MONITOR_DOWN; direction++) {
        if (monitor_move_possible(desktop, window, (monitor_direction) direction)) {
            return true;
        }
    }
    return false;
}

static bool is_transient(const wm_window *window)
{
    return window->parent != NULL;
}

static wm_menu_mark radio(bool on)
{
    return on ? WM_MENU_MARK_RADIO_ON : WM_MENU_MARK_RADIO_OFF;
}

static void build_workspaces(const wm_desktop *desktop, const wm_window *window,
                             wm_menu_level *level)
{
    bool sensitive = !is_transient(window) && window_has_flag(window, WINDOW_HAS_STICK)
                     && !window_is_sticky(window);
    for (int i = 0; i < desktop->workspaces.count && i < WM_MENU_ITEM_MAX; i++) {
        const char *name = desktop->config.workspaces.names[i];
        level->items[level->count++] = (wm_menu_item) {
            WM_MENU_WORKSPACE_ENTRY, name[0] != '\0' ? name : NULL,
            sensitive && i != window->workspace, false, WM_MENU_MARK_NONE};
    }
}

static void build_monitors(const wm_desktop *desktop, const wm_window *window,
                           wm_menu_level *level)
{
    static const wm_menu_label labels[] = {WM_MENU_MONITOR_LEFT, WM_MENU_MONITOR_RIGHT,
                                           WM_MENU_MONITOR_UP, WM_MENU_MONITOR_DOWN};
    for (int direction = MONITOR_LEFT; direction <= MONITOR_DOWN; direction++) {
        if (monitor_move_possible(desktop, window, (monitor_direction) direction)) {
            level->items[level->count++] = (wm_menu_item) {labels[direction], NULL, true,
                                                           false, WM_MENU_MARK_NONE};
        }
    }
}
