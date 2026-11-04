#include "output_color.h"

#include <drm_fourcc.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include "output_private.h"

static bool wants_ten_bit(const output *output);
static void apply_color_depth(output *output);

void output_color_describe(output *o, struct wlr_output_state *state)
{
    wlr_output_state_set_render_format(state, DRM_FORMAT_XRGB8888);
    if (!wants_ten_bit(o)) {
        return;
    }
    wlr_output_state_set_render_format(state, DRM_FORMAT_XRGB2101010);
    if (!wlr_output_test_state(o->wlr_output, state)) {
        wlr_log(WLR_INFO, "output %s: 10-bit color not possible, using 8-bit",
                o->wlr_output->name);
        wlr_output_state_set_render_format(state, DRM_FORMAT_XRGB8888);
    }
}

void outputs_apply_color_depth(outputs *outs)
{
    for (size_t i = 0; i < outputs_count(outs); i++) {
        apply_color_depth(outputs_at_index(outs, i));
    }
}

static bool wants_ten_bit(const output *o)
{
    return o->server->config.compositing.ten_bit_color;
}

static void apply_color_depth(output *o)
{
    uint32_t wanted = wants_ten_bit(o) ? DRM_FORMAT_XRGB2101010 : DRM_FORMAT_XRGB8888;
    if (o->wlr_output->render_format == wanted) {
        return;
    }
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    output_color_describe(o, &state);
    if (!wlr_output_commit_state(o->wlr_output, &state)) {
        wlr_log(WLR_ERROR, "output %s: cannot change color depth", o->wlr_output->name);
    }
    wlr_output_state_finish(&state);
}
