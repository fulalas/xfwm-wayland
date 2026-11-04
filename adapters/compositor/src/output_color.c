#include "output_color.h"

#include <drm_fourcc.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include "output_private.h"

static void apply_color_depth(output *output);

uint32_t output_color_wanted_format(const compositor *server)
{
    return server->config.compositing.ten_bit_color ? DRM_FORMAT_XRGB2101010
                                                    : output_color_fallback_format();
}

uint32_t output_color_fallback_format(void)
{
    return DRM_FORMAT_XRGB8888;
}

void output_color_describe(output *o, struct wlr_output_state *state)
{
    uint32_t wanted = output_color_wanted_format(o->server);
    uint32_t fallback = output_color_fallback_format();
    wlr_output_state_set_render_format(state, wanted);
    if (wanted == fallback || wlr_output_test_state(o->wlr_output, state)) {
        return;
    }
    wlr_log(WLR_INFO, "output %s: 10-bit color not possible, using 8-bit", o->wlr_output->name);
    wlr_output_state_set_render_format(state, fallback);
}

void outputs_apply_color_depth(outputs *outs)
{
    for (size_t i = 0; i < outputs_count(outs); i++) {
        apply_color_depth(outputs_at_index(outs, i));
    }
}

static void apply_color_depth(output *o)
{
    if (!o->wlr_output->enabled) {
        return;
    }
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    output_color_describe(o, &state);
    if (state.render_format != o->wlr_output->render_format &&
        !wlr_output_commit_state(o->wlr_output, &state)) {
        wlr_log(WLR_ERROR, "output %s: cannot change color depth", o->wlr_output->name);
    }
    wlr_output_state_finish(&state);
}
