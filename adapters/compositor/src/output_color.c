#include "output_color.h"

#include <drm_fourcc.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include "color_plan.h"
#include "output_private.h"

static const struct wlr_output_image_description hdr10 = {
    .primaries = WLR_COLOR_NAMED_PRIMARIES_BT2020,
    .transfer_function = WLR_COLOR_TRANSFER_FUNCTION_ST2084_PQ,
};

static color_mode wanted_mode(const output *output);
static size_t first_working(const output *output, const struct wlr_output_state *base,
                            const color_plan *plan);
static bool attempt_works(const output *output, const struct wlr_output_state *base,
                          const color_mode *mode);
static bool can_show_hdr(const output *output);
static void apply_mode(const output *output, struct wlr_output_state *state,
                       const color_mode *mode);
static void log_fallback(const output *output, const color_mode *wanted,
                         const color_mode *chosen);
static bool apply_color(output *output);
static bool color_changed(const output *output, const struct wlr_output_state *state);

void output_color_describe(output *o, struct wlr_output_state *state)
{
    color_plan plan = color_plan_for(wanted_mode(o));
    size_t chosen = first_working(o, state, &plan);
    apply_mode(o, state, &plan.attempts[chosen]);
    log_fallback(o, &plan.attempts[0], &plan.attempts[chosen]);
}

void output_color_describe_basic(output *o, struct wlr_output_state *state)
{
    color_mode basic = {.hdr = false, .ten_bit = false};
    apply_mode(o, state, &basic);
}

bool outputs_apply_color(outputs *outs)
{
    bool all_ok = true;
    for (size_t i = 0; i < outputs_count(outs); i++) {
        all_ok = apply_color(outputs_at_index(outs, i)) && all_ok;
    }
    return all_ok;
}

static color_mode wanted_mode(const output *o)
{
    const wm_compositing_config *config = &o->server->config.compositing;
    return (color_mode) {.hdr = config->hdr && can_show_hdr(o), .ten_bit = config->ten_bit_color};
}

static size_t first_working(const output *o, const struct wlr_output_state *base,
                            const color_plan *plan)
{
    size_t last = plan->count - 1;
    for (size_t i = 0; i < last; i++) {
        if (attempt_works(o, base, &plan->attempts[i])) {
            return i;
        }
    }
    return last;
}

static bool attempt_works(const output *o, const struct wlr_output_state *base,
                          const color_mode *mode)
{
    struct wlr_output_state attempt;
    wlr_output_state_init(&attempt);
    bool ok = wlr_output_state_copy(&attempt, base);
    if (ok) {
        apply_mode(o, &attempt, mode);
        ok = wlr_output_test_state(o->wlr_output, &attempt);
    }
    wlr_output_state_finish(&attempt);
    return ok;
}

static bool can_show_hdr(const output *o)
{
    const struct wlr_output *w = o->wlr_output;
    return o->server->renderer->features.output_color_transform &&
           (w->supported_transfer_functions & WLR_COLOR_TRANSFER_FUNCTION_ST2084_PQ) != 0 &&
           (w->supported_primaries & WLR_COLOR_NAMED_PRIMARIES_BT2020) != 0;
}

static void apply_mode(const output *o, struct wlr_output_state *state, const color_mode *mode)
{
    const struct wlr_output *w = o->wlr_output;
    uint32_t format = mode->ten_bit ? DRM_FORMAT_XRGB2101010 : DRM_FORMAT_XRGB8888;
    bool hdr_changes = mode->hdr != (w->image_description != NULL);
    wlr_output_state_set_render_format(state, format);
    if (hdr_changes) {
        wlr_output_state_set_image_description(state, mode->hdr ? &hdr10 : NULL);
    }
    if (hdr_changes || format != w->render_format) {
        state->allow_reconfiguration = true;
    }
}

static void log_fallback(const output *o, const color_mode *wanted, const color_mode *chosen)
{
    const char *name = o->wlr_output->name;
    if (wanted->hdr && !chosen->hdr) {
        wlr_log(WLR_INFO, "output %s: HDR not possible, using SDR", name);
    }
    if (wanted->ten_bit && !chosen->ten_bit) {
        wlr_log(WLR_INFO, "output %s: 10-bit color not possible, using 8-bit", name);
    }
}

static bool apply_color(output *o)
{
    if (!o->wlr_output->enabled) {
        return true;
    }
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    output_color_describe(o, &state);
    bool ok = !color_changed(o, &state) || wlr_output_commit_state(o->wlr_output, &state);
    if (!ok) {
        wlr_log(WLR_ERROR, "output %s: cannot change color mode", o->wlr_output->name);
    }
    wlr_output_state_finish(&state);
    return ok;
}

static bool color_changed(const output *o, const struct wlr_output_state *state)
{
    return state->render_format != o->wlr_output->render_format ||
           (state->committed & WLR_OUTPUT_STATE_IMAGE_DESCRIPTION) != 0;
}
