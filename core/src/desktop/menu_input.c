#include "menu.h"

#include "desktop_state.h"
#include "keybindings.h"

typedef struct mnemonic_matches {
    int first;
    int count;
} mnemonic_matches;

static void cancel(wm_desktop *desktop);
static void activate_selected(wm_desktop *desktop);
static wm_menu_level *active_level(menu_session *menu);
static bool is_selectable(const wm_menu_item *item);
static void step_selection(menu_session *menu, int direction);
static void open_submenu(wm_desktop *desktop);
static void close_submenu(menu_session *menu);
static void activate_main(wm_desktop *desktop, int index);
static void activate_submenu(wm_desktop *desktop, int index);
static void close_and_perform(wm_desktop *desktop, const wm_menu_item *item, int index);
static void hover_main(wm_desktop *desktop, int index);
static void handle_navigation(wm_desktop *desktop, uint32_t keysym);
static bool handle_mnemonic(wm_desktop *desktop, uint32_t keysym);
static mnemonic_matches find_mnemonic(const wm_desktop *desktop, const wm_menu_level *level,
                                      uint32_t keysym);

wm_dispatch menu_key(wm_desktop *desktop, const wm_key_event *event)
{
    if (keybindings_is_key_of(&desktop->keys, WM_ACTION_CANCEL, event->keysym)) {
        cancel(desktop);
        return WM_CONSUMED;
    }
    menu_show_mnemonics(desktop);
    if ((event->modifiers & (WM_MOD_CTRL | WM_MOD_SUPER)) == 0
        && handle_mnemonic(desktop, event->keysym)) {
        return WM_CONSUMED;
    }
    if (keysym_is_return(event->keysym)) {
        activate_selected(desktop);
        return WM_CONSUMED;
    }
    handle_navigation(desktop, event->keysym);
    return WM_CONSUMED;
}

bool menu_key_repeats(const wm_desktop *desktop, const wm_key_event *event)
{
    return keybindings_direction_of(&desktop->keys, event->keysym) != DIRECTION_NONE;
}

static void cancel(wm_desktop *desktop)
{
    if (desktop->menu.view.submenu_open) {
        close_submenu(&desktop->menu);
    } else {
        menu_close(desktop);
    }
}

static void activate_selected(wm_desktop *desktop)
{
    const menu_session *menu = &desktop->menu;
    if (menu->view.submenu_open && menu->view.submenu.selected >= 0) {
        activate_submenu(desktop, menu->view.submenu.selected);
    } else if (menu->view.main.selected >= 0) {
        activate_main(desktop, menu->view.main.selected);
    }
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
        desktop->menu.pressed_on_menu = on_menu;
        return WM_CONSUMED;
    }
    if (!desktop->menu.pressed_on_menu) {
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
    menu->view.submenu.mnemonics = menu->view.main.mnemonics;
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
    close_and_perform(desktop, &item, index);
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
    close_and_perform(desktop, &item, index);
}

static void close_and_perform(wm_desktop *desktop, const wm_menu_item *item, int index)
{
    menu_choice choice = {desktop->menu.window, *item, index};
    menu_close(desktop);
    menu_perform(desktop, &choice);
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

void menu_show_mnemonics(wm_desktop *desktop)
{
    menu_session *menu = &desktop->menu;
    if (!menu->view.main.mnemonics) {
        menu->view.main.mnemonics = true;
        menu->view.submenu.mnemonics = true;
        menu->dirty = true;
    }
}

static bool handle_mnemonic(wm_desktop *desktop, uint32_t keysym)
{
    menu_session *menu = &desktop->menu;
    wm_menu_level *level = active_level(menu);
    mnemonic_matches matches = find_mnemonic(desktop, level, keysym);
    if (matches.count == 0) {
        return false;
    }
    if (matches.count > 1) {
        level->selected = matches.first;
        menu->dirty = true;
    } else if (menu->view.submenu_open) {
        activate_submenu(desktop, matches.first);
    } else {
        activate_main(desktop, matches.first);
    }
    return true;
}

static mnemonic_matches find_mnemonic(const wm_desktop *desktop, const wm_menu_level *level,
                                      uint32_t keysym)
{
    const wm_environment *environment = &desktop->ports.environment;
    int count = (int) level->count;
    mnemonic_matches matches = {-1, 0};
    for (int step = 1; step <= count; step++) {
        int index = (level->selected + step) % count;
        wm_menu_entry entry = {level, (size_t) index};
        if (is_selectable(&level->items[index])
            && environment->menu_item_matches_key(environment->self, &entry, keysym)) {
            matches.first = matches.first < 0 ? index : matches.first;
            matches.count++;
        }
    }
    return matches;
}
