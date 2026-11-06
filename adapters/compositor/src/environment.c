#include <xkbcommon/xkbcommon.h>

#include "clock.h"
#include "output.h"
#include "output_setting.h"
#include "seat.h"
#include "server.h"

static size_t environment_output_count(void *self);
static wm_output_info environment_output_at_index(void *self, size_t index);
static wm_point environment_pointer_position(void *self);
static uint32_t environment_now_ms(void *self);
static wm_size environment_measure_title(void *self, const char *text);
static bool environment_menu_item_matches_key(void *self, const wm_menu_level *level,
                                              size_t index, uint32_t keysym);

wm_environment compositor_environment(compositor *c)
{
    wm_environment environment = {
        .self = c,
        .output_count = environment_output_count,
        .output_at_index = environment_output_at_index,
        .pointer_position = environment_pointer_position,
        .now_ms = environment_now_ms,
        .measure_title = environment_measure_title,
        .menu_item_matches_key = environment_menu_item_matches_key,
    };
    return environment;
}

static size_t environment_output_count(void *self)
{
    compositor *c = self;
    return c->outputs != NULL ? outputs_count(c->outputs) : 0;
}

static wm_output_info environment_output_at_index(void *self, size_t index)
{
    compositor *c = self;
    output *o = c->outputs != NULL ? outputs_at_index(c->outputs, index) : NULL;
    if (o == NULL) {
        return (wm_output_info){{0, 0, 0, 0}, {0, 0, 0, 0}, false};
    }
    return (wm_output_info){output_full(o), output_usable(o), output_display_of(o)->is_primary};
}

static wm_point environment_pointer_position(void *self)
{
    compositor *c = self;
    return c->seat != NULL ? seat_pointer_position(c->seat) : (wm_point){0, 0};
}

static uint32_t environment_now_ms(void *self)
{
    (void) self;
    return (uint32_t) clock_now_ms();
}

static wm_size environment_measure_title(void *self, const char *text)
{
    compositor *c = self;
    return c->painter->measure_title(c->painter->self, text);
}

static bool environment_menu_item_matches_key(void *self, const wm_menu_level *level,
                                              size_t index, uint32_t keysym)
{
    compositor *c = self;
    const compositor_osd_painter *painter = &c->painter->osd;
    uint32_t typed = xkb_keysym_to_utf32(xkb_keysym_to_lower(keysym));
    return typed != 0 && painter->menu_mnemonic(painter->self, level, index) == typed;
}
