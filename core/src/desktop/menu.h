#ifndef DESKTOP_MENU_H
#define DESKTOP_MENU_H

#include <stdbool.h>

#include "wm/desktop.h"

typedef struct menu_session {
    bool active;
    bool shown;
    bool dirty;
    bool pressed_on_menu;
    wm_window *window;
    wm_menu_view view;
} menu_session;

typedef struct menu_choice {
    wm_window *window;
    wm_menu_item item;
    int index;
} menu_choice;

void menu_show(wm_desktop *desktop, wm_window *window, wm_point point);
bool menu_is_active(const wm_desktop *desktop);
void menu_close(wm_desktop *desktop);
void menu_window_gone(wm_desktop *desktop, const wm_window *window);
void menu_present(wm_desktop *desktop);
void menu_build(const wm_desktop *desktop, const wm_window *window, wm_menu_level *level);
void menu_build_submenu(const wm_desktop *desktop, wm_menu_label label, wm_menu_level *level);
void menu_perform(wm_desktop *desktop, const menu_choice *choice);

wm_dispatch menu_key(wm_desktop *desktop, const wm_key_event *event);
wm_dispatch menu_motion(wm_desktop *desktop, const wm_pointer_event *event);
wm_dispatch menu_button(wm_desktop *desktop, const wm_button_event *event);
void menu_show_mnemonics(wm_desktop *desktop);

#endif
