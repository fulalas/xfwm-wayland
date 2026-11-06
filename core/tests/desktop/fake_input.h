#ifndef FAKE_INPUT_H
#define FAKE_INPUT_H

#include "fake_ports.h"

typedef struct fake_click {
    wm_target_kind kind;
    wm_window *window;
    wm_point point;
    uint32_t button;
    uint32_t modifiers;
} fake_click;

wm_dispatch fake_motion(fake_world *world, wm_desktop *desktop, const fake_click *where);
wm_dispatch fake_press(fake_world *world, wm_desktop *desktop, const fake_click *click);
wm_dispatch fake_release(fake_world *world, wm_desktop *desktop, const fake_click *click);
wm_dispatch fake_scroll(fake_world *world, wm_desktop *desktop, const fake_click *where,
                        int vertical_steps);
wm_dispatch fake_key(wm_desktop *desktop, uint32_t keysym, uint32_t modifiers);
wm_dispatch fake_key_release(wm_desktop *desktop, uint32_t keysym, uint32_t modifiers);
wm_point fake_part_point(const wm_window *window, wm_part part);
fake_click fake_on_part(wm_window *window, wm_part part, uint32_t button);
void fake_set_keymap(wm_desktop *desktop);
wm_desktop *fake_desktop_start(fake_world *world, const wm_config *config);

enum fake_keysym {
    FAKE_KEY_A = 0x61,
    FAKE_KEY_C = 0x63,
    FAKE_KEY_F = 0x66,
    FAKE_KEY_M = 0x6d,
    FAKE_KEY_N = 0x6e,
    FAKE_KEY_R = 0x72,
    FAKE_KEY_T = 0x74,
    FAKE_KEY_F4 = 0xffc1,
    FAKE_KEY_F7 = 0xffc4,
    FAKE_KEY_F8 = 0xffc5,
    FAKE_KEY_SPACE = 0x20,
    FAKE_KEY_ALT_L = 0xffe9,
    FAKE_KEY_1 = 0x31,
    FAKE_KEY_2 = 0x32,
    FAKE_KEY_D = 0x64,
    FAKE_KEY_X = 0x78,
};

enum {
    FAKE_APP_SHORTCUT = 7,
};

#endif
