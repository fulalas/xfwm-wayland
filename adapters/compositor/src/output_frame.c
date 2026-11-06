#include <wayland-protocols/tearing-control-v1-enum.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_tearing_control_v1.h>

#include "output_brightness.h"
#include "output_private.h"
#include "output_repaint.h"
#include "scene_buffers.h"
#include "seat.h"
#include "view.h"
#include "zoom.h"

static void output_render_scene(output *output);
static bool focused_view_wants_tearing(const output *output);

void output_render(output *o)
{
    output_render_frame(o);
    output_send_frame_done(o);
}

void output_render_frame(output *o)
{
    if (o->scene_output == NULL) {
        return;
    }
    if (zoom_is_active(o->server->zoom)) {
        zoom_render(o->server->zoom, o);
    } else {
        output_render_scene(o);
    }
}

void output_send_frame_done(output *o)
{
    if (o->scene_output == NULL) {
        return;
    }
    scene_output_send_frame_done_now(o->scene_output);
}

void output_tell_scene_color(const output *o, struct wlr_output_state *state)
{
    const struct wlr_output_image_description *now = o->wlr_output->image_description;
    if ((now != NULL) != o->scene_hdr) {
        wlr_output_state_set_image_description(state, now);
    }
}

bool output_commit_state(output *o, struct wlr_output_state *state)
{
    bool painted = (state->committed & WLR_OUTPUT_STATE_BUFFER) != 0;
    state->tearing_page_flip = painted && output_wants_tearing(o);
    if (state->tearing_page_flip && !wlr_output_test_state(o->wlr_output, state)) {
        state->tearing_page_flip = false;
    }
    if (!wlr_output_commit_state(o->wlr_output, state)) {
        return false;
    }
    if (painted) {
        paint_stats_note(&o->stats, o->wlr_output->name);
    }
    if ((state->committed & WLR_OUTPUT_STATE_IMAGE_DESCRIPTION) != 0) {
        o->scene_hdr = state->image_description != NULL;
    }
    return true;
}

static void output_render_scene(output *o)
{
    if (!wlr_scene_output_needs_frame(o->scene_output)) {
        return;
    }
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    struct wlr_scene_output_state_options options = {.timer = output_repaint_timer(o)};
    output_tell_scene_color(o, &state);
    output_brightness_frame(o, &state);
    if (wlr_scene_output_build_state(o->scene_output, &state, &options)) {
        output_commit_state(o, &state);
    }
    wlr_output_state_finish(&state);
}

bool output_wants_tearing(const output *o)
{
    if (o->server->config.compositing.vblank_mode == WM_VBLANK_OFF) {
        return true;
    }
    return focused_view_wants_tearing(o);
}

static bool focused_view_wants_tearing(const output *o)
{
    compositor *server = o->server;
    view *focused = seat_focused_view(server->seat);
    if (focused == NULL || !view_is_fullscreen(focused) || server->globals.tearing == NULL) {
        return false;
    }
    wm_rect client = view_client_rect(focused);
    struct wlr_surface *surface = view_surface(focused);
    if (surface == NULL || outputs_for_rect(server->outputs, &client) != o) {
        return false;
    }
    return wlr_tearing_control_manager_v1_surface_hint_from_surface(server->globals.tearing,
                                                                    surface) ==
           WP_TEARING_CONTROL_V1_PRESENTATION_HINT_ASYNC;
}
