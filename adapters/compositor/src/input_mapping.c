#include <linux/input-event-codes.h>
#include <stddef.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/util/edges.h>
#include <xkbcommon/xkbcommon.h>

#include "input_mapping.h"
#include "wm/desktop.h"
#include "wm/keys.h"

enum {
    WHEEL_STEP_V120 = 120,
    VT_COUNT = 12,
};

typedef struct code_pair {
    uint32_t from;
    uint32_t to;
} code_pair;

static const code_pair modifier_pairs[] = {
    {WLR_MODIFIER_SHIFT, WM_MOD_SHIFT}, {WLR_MODIFIER_CTRL, WM_MOD_CTRL},
    {WLR_MODIFIER_ALT, WM_MOD_ALT},     {WLR_MODIFIER_LOGO, WM_MOD_SUPER},
    {WLR_MODIFIER_MOD3, WM_MOD_MOD3},   {WLR_MODIFIER_MOD5, WM_MOD_MOD5},
};

static const code_pair button_pairs[] = {
    {BTN_LEFT, WM_MOUSE_LEFT}, {BTN_MIDDLE, WM_MOUSE_MIDDLE}, {BTN_RIGHT, WM_MOUSE_RIGHT},
    {BTN_SIDE, WM_MOUSE_BACK}, {BTN_EXTRA, WM_MOUSE_FORWARD},
};

static const code_pair edge_pairs[] = {
    {WLR_EDGE_TOP, WM_EDGE_TOP},
    {WLR_EDGE_BOTTOM, WM_EDGE_BOTTOM},
    {WLR_EDGE_LEFT, WM_EDGE_LEFT},
    {WLR_EDGE_RIGHT, WM_EDGE_RIGHT},
};

static uint32_t map_bits(const code_pair *pairs, size_t count, uint32_t bits);
static uint32_t map_bits_back(const code_pair *pairs, size_t count, uint32_t bits);

uint32_t input_mapping_modifiers(uint32_t wlr_modifiers)
{
    size_t count = sizeof(modifier_pairs) / sizeof(modifier_pairs[0]);
    return map_bits(modifier_pairs, count, wlr_modifiers);
}

uint32_t input_mapping_keysym(uint32_t level0_keysym)
{
    return xkb_keysym_to_lower(level0_keysym);
}

unsigned input_mapping_vt(const key_chord *chord)
{
    uint32_t sym = chord->translated_keysym;
    if (sym >= XKB_KEY_XF86Switch_VT_1 && sym < XKB_KEY_XF86Switch_VT_1 + VT_COUNT) {
        return sym - XKB_KEY_XF86Switch_VT_1 + 1;
    }
    uint32_t wanted = WM_MOD_CTRL | WM_MOD_ALT;
    uint32_t base = chord->base_keysym;
    if ((chord->modifiers & wanted) == wanted && base >= XKB_KEY_F1 &&
        base < XKB_KEY_F1 + VT_COUNT) {
        return base - XKB_KEY_F1 + 1;
    }
    return 0;
}

uint32_t input_mapping_button(uint32_t linux_button)
{
    for (size_t i = 0; i < sizeof(button_pairs) / sizeof(button_pairs[0]); i++) {
        if (button_pairs[i].from == linux_button) {
            return button_pairs[i].to;
        }
    }
    return 0;
}

axis_steps input_mapping_axis(int32_t remainder, int32_t value120)
{
    int32_t total = remainder + value120;
    axis_steps result = {
        .remainder = total % WHEEL_STEP_V120,
        .steps = total / WHEEL_STEP_V120,
    };
    return result;
}

uint32_t input_mapping_edges_from_wlr(uint32_t wlr_edges)
{
    return map_bits(edge_pairs, sizeof(edge_pairs) / sizeof(edge_pairs[0]), wlr_edges);
}

uint32_t input_mapping_edges_to_wlr(uint32_t wm_edges)
{
    return map_bits_back(edge_pairs, sizeof(edge_pairs) / sizeof(edge_pairs[0]), wm_edges);
}

static uint32_t map_bits(const code_pair *pairs, size_t count, uint32_t bits)
{
    uint32_t result = 0;
    for (size_t i = 0; i < count; i++) {
        if ((bits & pairs[i].from) != 0) {
            result |= pairs[i].to;
        }
    }
    return result;
}

static uint32_t map_bits_back(const code_pair *pairs, size_t count, uint32_t bits)
{
    uint32_t result = 0;
    for (size_t i = 0; i < count; i++) {
        if ((bits & pairs[i].to) != 0) {
            result |= pairs[i].from;
        }
    }
    return result;
}
