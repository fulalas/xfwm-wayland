#include "fake_input.h"

static wm_pointer_event pointer_event(fake_world *world, const fake_click *click);

wm_dispatch fake_motion(fake_world *world, wm_desktop *desktop, const fake_click *where)
{
    wm_pointer_event event = pointer_event(world, where);
    return wm_pointer_motion(desktop, &event);
}

wm_dispatch fake_press(fake_world *world, wm_desktop *desktop, const fake_click *click)
{
    wm_button_event event = {pointer_event(world, click), click->button, true};
    return wm_pointer_button(desktop, &event);
}

wm_dispatch fake_release(fake_world *world, wm_desktop *desktop, const fake_click *click)
{
    wm_button_event event = {pointer_event(world, click), click->button, false};
    return wm_pointer_button(desktop, &event);
}

wm_dispatch fake_scroll(fake_world *world, wm_desktop *desktop, const fake_click *where,
                        int vertical_steps)
{
    wm_axis_event event = {pointer_event(world, where), vertical_steps, 0};
    return wm_pointer_axis(desktop, &event);
}

wm_dispatch fake_key(wm_desktop *desktop, uint32_t keysym, uint32_t modifiers)
{
    wm_key_event event = {keysym, modifiers, 0};
    return wm_key_pressed(desktop, &event);
}

wm_point fake_part_point(const wm_window *window, wm_part part)
{
    wm_window_view view = wm_window_current_view(window);
    for (size_t i = 0; i < view.decor->part_count; i++) {
        if (view.decor->parts[i].part == part) {
            wm_rect rect = view.decor->parts[i].rect;
            return (wm_point) {view.frame.x + rect.x + rect.width / 2,
                               view.frame.y + rect.y + rect.height / 2};
        }
    }
    return (wm_point) {view.client.x + view.client.width / 2,
                       view.client.y + view.client.height / 2};
}

fake_click fake_on_part(wm_window *window, wm_part part, uint32_t button)
{
    fake_click click = {WM_TARGET_FRAME, window, fake_part_point(window, part), button, 0};
    return click;
}

void fake_set_keymap(wm_desktop *desktop)
{
    static const wm_key_binding bindings[] = {
        {{WM_KEYSYM_ESCAPE, 0}, WM_ACTION_CANCEL},
        {{WM_KEYSYM_DOWN, 0}, WM_ACTION_DOWN},
        {{WM_KEYSYM_TAB, WM_MOD_ALT}, WM_ACTION_CYCLE_WINDOWS},
        {{WM_KEYSYM_TAB, WM_MOD_ALT | WM_MOD_SHIFT}, WM_ACTION_CYCLE_REVERSE_WINDOWS},
        {{FAKE_KEY_F4, WM_MOD_ALT}, WM_ACTION_CLOSE_WINDOW},
        {{FAKE_KEY_F7, WM_MOD_ALT}, WM_ACTION_MOVE},
        {{FAKE_KEY_F8, WM_MOD_ALT}, WM_ACTION_RESIZE},
        {{FAKE_KEY_SPACE, WM_MOD_ALT}, WM_ACTION_POPUP_MENU},
        {{WM_KEYSYM_LEFT, WM_MOD_SUPER}, WM_ACTION_TILE_LEFT},
        {{WM_KEYSYM_RIGHT, WM_MOD_CTRL | WM_MOD_ALT}, WM_ACTION_RIGHT_WORKSPACE},
        {{WM_KEYSYM_LEFT, WM_MOD_CTRL | WM_MOD_ALT}, WM_ACTION_LEFT_WORKSPACE},
        {{FAKE_KEY_1, WM_MOD_CTRL}, WM_ACTION_WORKSPACE_1},
        {{FAKE_KEY_2, WM_MOD_CTRL}, WM_ACTION_WORKSPACE_1 + 1},
        {{FAKE_KEY_2, WM_MOD_ALT | WM_MOD_CTRL}, WM_ACTION_MOVE_WORKSPACE_1 + 1},
        {{FAKE_KEY_D, WM_MOD_CTRL | WM_MOD_ALT}, WM_ACTION_SHOW_DESKTOP},
        {{FAKE_KEY_A, WM_MOD_SUPER}, WM_ACTION_ADD_WORKSPACE},
        {{FAKE_KEY_X, WM_MOD_SUPER}, WM_ACTION_DEL_ACTIVE_WORKSPACE},
        {{FAKE_KEY_F, WM_MOD_SUPER}, WM_ACTION_FILL_WINDOW},
        {{FAKE_KEY_M, WM_MOD_SUPER}, WM_ACTION_MAXIMIZE_WINDOW},
    };
    static const wm_command_binding commands[] = {
        {{FAKE_KEY_T, WM_MOD_CTRL | WM_MOD_ALT}, "xfce4-terminal"},
    };
    wm_keymap keymap = {bindings, sizeof(bindings) / sizeof(bindings[0]), commands, 1};
    wm_desktop_set_keymap(desktop, &keymap);
}

wm_desktop *fake_desktop_start(fake_world *world, const wm_config *config)
{
    fake_world_init(world);
    wm_desktop *desktop = fake_desktop_create(world, config);
    fake_set_keymap(desktop);
    return desktop;
}

static wm_pointer_event pointer_event(fake_world *world, const fake_click *click)
{
    world->pointer = click->point;
    wm_pointer_event event = {click->point, world->now_ms, click->modifiers,
                              {click->kind, click->window, -1}};
    return event;
}
