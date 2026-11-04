
#include "clock.h"
#include "output.h"
#include "seat.h"
#include "server.h"

static size_t environment_output_count(void *self);
static wm_output_info environment_output_at_index(void *self, size_t index);
static wm_point environment_pointer_position(void *self);
static uint32_t environment_now_ms(void *self);
static wm_size environment_measure_title(void *self, const char *text);

wm_environment compositor_environment(compositor *c)
{
    wm_environment environment = {
        .self = c,
        .output_count = environment_output_count,
        .output_at_index = environment_output_at_index,
        .pointer_position = environment_pointer_position,
        .now_ms = environment_now_ms,
        .measure_title = environment_measure_title,
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
        return (wm_output_info){{0, 0, 0, 0}, {0, 0, 0, 0}};
    }
    return (wm_output_info){output_full(o), output_usable(o)};
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
