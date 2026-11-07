#include "output_brightness.h"

#include <stdlib.h>
#include <wlr/interfaces/wlr_output.h>
#include <wlr/render/color.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include "brightness_curve.h"
#include "output_private.h"

enum {
    NEUTRAL_PERCENT = 100,
    FOREIGN_PERCENT = -1,
};

static int wanted_percent(const output *output);
static bool needs_change(const output *output);
static void request_frame(output *output);
static struct wlr_color_transform *curve_for(output *output, int percent);
static struct wlr_color_transform *build_curve(const output *output, int percent);
static void forget_color_transform(struct wlr_output_state *state);
static void note_failure(output *output, int percent);

void output_brightness_frame(output *o, struct wlr_output_state *state)
{
    if (!needs_change(o)) {
        return;
    }
    int wanted = wanted_percent(o);
    struct wlr_color_transform *curve = wanted == NEUTRAL_PERCENT ? NULL : curve_for(o, wanted);
    if (wanted != NEUTRAL_PERCENT && curve == NULL) {
        note_failure(o, wanted);
        return;
    }
    wlr_output_state_set_color_transform(state, curve);
    if (!wlr_output_test_state(o->wlr_output, state)) {
        forget_color_transform(state);
        note_failure(o, wanted);
    }
}

void output_brightness_committed(output *o, const struct wlr_output_state *state)
{
    if ((state->committed & WLR_OUTPUT_STATE_COLOR_TRANSFORM) != 0) {
        const struct wlr_color_transform *now = state->color_transform;
        o->brightness.on_screen = now == NULL                 ? NEUTRAL_PERCENT
                                  : now == o->brightness.curve ? o->brightness.curve_percent
                                                               : FOREIGN_PERCENT;
    }
    request_frame(o);
}

void output_brightness_init(output *o)
{
    o->brightness = (output_brightness) {.on_screen = NEUTRAL_PERCENT};
}

void output_brightness_release(output *o)
{
    wlr_color_transform_unref(o->brightness.curve);
    output_brightness_init(o);
}

void outputs_brightness_changed(outputs *outs)
{
    for (output *o = outputs_next_active(outs, NULL); o != NULL; o = outputs_next_active(outs, o)) {
        request_frame(o);
    }
}

static int wanted_percent(const output *o)
{
    bool hdr = o->wlr_output->image_description != NULL;
    return hdr ? o->server->config.compositing.hdr_brightness_percent : NEUTRAL_PERCENT;
}

static bool needs_change(const output *o)
{
    int wanted = wanted_percent(o);
    const output_brightness *b = &o->brightness;
    return o->wlr_output->enabled && b->on_screen != FOREIGN_PERCENT && wanted != b->on_screen &&
           wanted != b->failed_percent;
}

static void request_frame(output *o)
{
    if (needs_change(o)) {
        wlr_output_update_needs_frame(o->wlr_output);
    }
}

static struct wlr_color_transform *curve_for(output *o, int percent)
{
    output_brightness *b = &o->brightness;
    if (b->curve == NULL || b->curve_percent != percent) {
        wlr_color_transform_unref(b->curve);
        b->curve = build_curve(o, percent);
        b->curve_percent = percent;
    }
    return b->curve;
}

static struct wlr_color_transform *build_curve(const output *o, int percent)
{
    size_t size = wlr_output_get_gamma_size(o->wlr_output);
    uint16_t *curve = size > 0 ? calloc(size, sizeof(*curve)) : NULL;
    if (curve == NULL) {
        return NULL;
    }
    brightness_curve_pq(percent / (double) NEUTRAL_PERCENT, curve, size);
    struct wlr_color_transform *transform =
        wlr_color_transform_init_lut_3x1d(size, curve, curve, curve);
    free(curve);
    return transform;
}

static void forget_color_transform(struct wlr_output_state *state)
{
    wlr_color_transform_unref(state->color_transform);
    state->color_transform = NULL;
    state->committed &= ~WLR_OUTPUT_STATE_COLOR_TRANSFORM;
}

static void note_failure(output *o, int percent)
{
    o->brightness.failed_percent = percent;
    wlr_log(WLR_INFO, "output %s: cannot set HDR brightness to %d%%", o->wlr_output->name,
            percent);
}
