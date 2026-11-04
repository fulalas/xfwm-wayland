#include "menu.h"

#include "desktop_state.h"
#include "keybindings.h"

static wm_menu_level *active_level(menu_session *menu);
static bool is_selectable(const wm_menu_item *item);
static void step_selection(menu_session *menu, int direction);
static void open_submenu(wm_desktop *desktop);
static void close_submenu(menu_session *menu);
static void activate_main(wm_desktop *desktop, int index);
static void activate_submenu(wm_desktop *desktop, int index);
static void hover_main(wm_desktop *desktop, int index);
static void handle_navigation(wm_desktop *desktop, uint32_t keysym);

wm_dispatch menu_key(wm_desktop *desktop, const wm_key_event *event)
{
    menu_session *menu = &desktop->menu;
    if (keybindings_is_key_of(&desktop->keys, WM_ACTION_CANCEL, event->keysym)) {
        if (menu->view.submenu_open) {
            close_submenu(menu);
        } else {
            menu_close(desktop);
        }
        return WM_CONSUMED;
    }
    if (keysym_is_return(event->keysym)) {
        if (menu->view.submenu_open && menu->view.submenu.selected >= 0) {
            activate_submenu(desktop, menu->view.submenu.selected);
        } else if (menu->view.main.selected >= 0) {
            activate_main(desktop, menu->view.main.selected);
        }
        return WM_CONSUMED;
    }
    handle_navigation(desktop, event->keysym);
    return WM_CONSUMED;
}

wm_dispatch menu_motion(wm_desktop *desktop, const wm_pointer_event *event)
{
    menu_session *menu = &desktop->menu;
    int item = event->target.item;
    if (event->target.kind == WM_TARGET_MENU && item >= 0
        && (size_t) item < menu->view.main.count && item != menu->view.main.selected) {
        hover_main(desktop, item);
    } else if (event->target.kind == WM_TARGET_SUBMENU && menu->view.submenu_open && item >= 0
               && (size_t) item < menu->view.submenu.count
               && item != menu->view.submenu.selected) {
        menu->view.submenu.selected = item;
        menu->dirty = true;
    }
    return WM_CONSUMED;
}

wm_dispatch menu_button(wm_desktop *desktop, const wm_button_event *event)
{
    wm_target_kind kind = event->pointer.target.kind;
    int item = event->pointer.target.item;
    bool on_menu = kind == WM_TARGET_MENU || kind == WM_TARGET_SUBMENU;
    if (event->pressed) {
        if (!on_menu) {
            menu_close(desktop);
        }
        return WM_CONSUMED;
    }
    if (kind == WM_TARGET_MENU && item >= 0) {
        activate_main(desktop, item);
    } else if (kind == WM_TARGET_SUBMENU && item >= 0) {
        activate_submenu(desktop, item);
    }
    return WM_CONSUMED;
}

static wm_menu_level *active_level(menu_session *menu)
{
    return menu->view.submenu_open ? &menu->view.submenu : &menu->view.main;
}

static bool is_selectable(const wm_menu_item *item)
{
    return item->label != WM_MENU_SEPARATOR && item->sensitive;
}

static void step_selection(menu_session *menu, int direction)
{
    wm_menu_level *level = active_level(menu);
    int count = (int) level->count;
    int index = level->selected;
    for (int i = 0; i < count; i++) {
        index = index < 0 && direction < 0 ? count - 1 : (index + direction + count) % count;
        if (is_selectable(&level->items[index])) {
            level->selected = index;
            menu->dirty = true;
            return;
        }
    }
}

static void open_submenu(wm_desktop *desktop)
{
    menu_session *menu = &desktop->menu;
    int index = menu->view.main.selected;
    if (index < 0 || !menu->view.main.items[index].has_submenu
        || !menu->view.main.items[index].sensitive) {
        return;
    }
    menu_build_submenu(desktop, menu->view.main.items[index].label, &menu->view.submenu);
    menu->view.submenu_open = true;
    menu->dirty = true;
}

static void close_submenu(menu_session *menu)
{
    menu->view.submenu_open = false;
    menu->view.submenu.count = 0;
    menu->dirty = true;
}

static void activate_main(wm_desktop *desktop, int index)
{
    menu_session *menu = &desktop->menu;
    if ((size_t) index >= menu->view.main.count) {
        return;
    }
    wm_menu_item item = menu->view.main.items[index];
    if (!is_selectable(&item)) {
        return;
    }
    if (item.has_submenu) {
        menu->view.main.selected = index;
        open_submenu(desktop);
        return;
    }
    wm_window *window = menu->window;
    menu_close(desktop);
    menu_perform(desktop, window, &item, index);
}

static void activate_submenu(wm_desktop *desktop, int index)
{
    menu_session *menu = &desktop->menu;
    if (!menu->view.submenu_open || (size_t) index >= menu->view.submenu.count) {
        return;
    }
    wm_menu_item item = menu->view.submenu.items[index];
    if (!is_selectable(&item)) {
        return;
    }
    wm_window *window = menu->window;
    menu_close(desktop);
    menu_perform(desktop, window, &item, index);
}

static void hover_main(wm_desktop *desktop, int index)
{
    menu_session *menu = &desktop->menu;
    menu->view.main.selected = index;
    menu->dirty = true;
    close_submenu(menu);
    open_submenu(desktop);
}

static void handle_navigation(wm_desktop *desktop, uint32_t keysym)
{
    menu_session *menu = &desktop->menu;
    const keybindings *keys = &desktop->keys;
    if (keybindings_is_key_of(keys, WM_ACTION_DOWN, keysym)) {
        step_selection(menu, 1);
    } else if (keybindings_is_key_of(keys, WM_ACTION_UP, keysym)) {
        step_selection(menu, -1);
    } else if (keybindings_is_key_of(keys, WM_ACTION_RIGHT, keysym)
               && !menu->view.submenu_open) {
        open_submenu(desktop);
        if (menu->view.submenu_open) {
            step_selection(menu, 1);
        }
    } else if (keybindings_is_key_of(keys, WM_ACTION_LEFT, keysym) && menu->view.submenu_open) {
        close_submenu(menu);
    }
}
