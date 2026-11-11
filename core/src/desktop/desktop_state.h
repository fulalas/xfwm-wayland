#ifndef DESKTOP_DESKTOP_STATE_H
#define DESKTOP_DESKTOP_STATE_H

#include "click.h"
#include "cycle.h"
#include "focus.h"
#include "frame_buttons.h"
#include "keybindings.h"
#include "launches.h"
#include "menu.h"
#include "monitors.h"
#include "moveresize.h"
#include "present.h"
#include "rules.h"
#include "timers.h"
#include "window.h"
#include "window_list.h"
#include "workspaces.h"
#include "wrap.h"
#include "zoom.h"

struct wm_desktop {
    wm_ports ports;
    wm_config config;
    wm_theme_metrics theme;
    keybindings keys;
    window_list stack;
    window_list mru;
    unsigned long next_serial;
    focus_state focus;
    workspace_state workspaces;
    timer_set timers;
    present_state present;
    move_session session;
    wrap_state wrap;
    click_state click;
    frame_buttons_state buttons;
    cycle_session cycle;
    menu_session menu;
    zoom_state zoom;
    monitor_memory remembered_monitors;
    launch_set launches;
    rule_set rules;
    wm_window *highlighted;
    wm_point pointer;
    uint32_t modifiers;
    bool has_user_input;
    uint32_t last_user_input_ms;
    uint32_t held_app_shortcut;
    uint32_t held_app_shortcut_keysym;
    wm_key_event tapped_modifier;
    bool has_scrolled;
    uint32_t last_scroll_ms;
};

#endif
