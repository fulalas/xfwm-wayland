#include "menu.h"

#include "desktop_state.h"
#include "frame.h"

static void set_menu_button(wm_window *window, wm_button_status status);

void menu_show(wm_desktop *desktop, wm_window *window, wm_point point)
{
    menu_session *menu = &desktop->menu;
    if (!window->visible) {
        return;
    }
    if (menu->active) {
        menu_close(desktop);
    }
    if (frame_is_shown(desktop, window)) {
        point.y = window->client.y;
    }
    menu->active = true;
    menu->dirty = true;
    menu->pressed_on_menu = false;
    menu->window = window;
    menu->view = (wm_menu_view) {0};
    menu_build(desktop, window, &menu->view.main);
    menu->view.main.anchor = point;
    menu->view.main.selected = -1;
    set_menu_button(window, WM_BUTTON_PRESSED);
}

bool menu_is_active(const wm_desktop *desktop)
{
    return desktop->menu.active;
}

void menu_close(wm_desktop *desktop)
{
    menu_session *menu = &desktop->menu;
    if (!menu->active) {
        return;
    }
    menu->active = false;
    set_menu_button(menu->window, WM_BUTTON_NORMAL);
    menu->window = NULL;
}

void menu_window_gone(wm_desktop *desktop, const wm_window *window)
{
    menu_session *menu = &desktop->menu;
    if (menu->active && menu->window == window) {
        menu->active = false;
        menu->window = NULL;
    }
}

void menu_present(wm_desktop *desktop)
{
    menu_session *menu = &desktop->menu;
    const wm_presenter *presenter = &desktop->ports.presenter;
    if (menu->active && menu->dirty) {
        presenter->menu_shown(presenter->self, &menu->view);
        menu->shown = true;
    } else if (!menu->active && menu->shown) {
        presenter->menu_hidden(presenter->self);
        menu->shown = false;
    }
    menu->dirty = false;
}

static void set_menu_button(wm_window *window, wm_button_status status)
{
    if (window->buttons[WM_BUTTON_MENU] != status) {
        window->buttons[WM_BUTTON_MENU] = status;
        window_mark_dirty(window);
    }
}
