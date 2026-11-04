#include "output_setting.h"

#include <stdlib.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include "mode_choice.h"
#include "output_private.h"

static const display_setting automatic_setting = {.enabled = true, .scale = 1.0};

static bool commit_setting(output *output, const display_setting *setting);
static bool commit_with_fallback(output *output, const display_setting *setting);
static bool wants_adaptive_sync(const output *output);
static void describe(struct wlr_output *wlr_output, const display_setting *setting,
                     struct wlr_output_state *state);
static void describe_mode(struct wlr_output *wlr_output, const display_mode *wanted,
                          struct wlr_output_state *state);
static struct wlr_output_mode *find_mode(struct wlr_output *wlr_output,
                                         const display_mode *wanted);
static int pick_mode(struct wlr_output *wlr_output, const display_mode *wanted);
static struct wlr_output_mode *mode_at(struct wlr_output *wlr_output, int index);
static display_setting output_current_setting(const output *output);

output_display *output_display_of(output *o)
{
    return &o->display;
}

display_identity output_display_identity(const output *o)
{
    struct wlr_output *w = o->wlr_output;
    return (display_identity){
        .connector = w->name,
        .description = w->description,
        .make = w->make,
        .model = w->model,
        .serial = w->serial,
        .edid = o->display.edid.data,
        .edid_size = o->display.edid.size,
    };
}

static display_setting output_current_setting(const output *o)
{
    struct wlr_output *w = o->wlr_output;
    wm_rect full = output_full(o);
    return (display_setting){
        .enabled = output_is_active(o),
        .mode = {w->width, w->height, w->refresh},
        .x = full.x,
        .y = full.y,
        .transform = (display_transform) w->transform,
        .scale = w->scale,
    };
}

display_setting output_saved_setting(const output *o)
{
    if (output_is_active(o)) {
        return output_current_setting(o);
    }
    if (o->display.wants_enabled && o->display.has_last_enabled) {
        return o->display.last_enabled;
    }
    return (display_setting){.enabled = false, .scale = 1.0};
}

bool output_apply_at(output *o, const display_setting *setting)
{
    if (!commit_with_fallback(o, setting)) {
        return false;
    }
    return output_place(o, (wm_point){setting->x, setting->y});
}

bool output_apply_auto(output *o)
{
    if (!commit_with_fallback(o, &automatic_setting)) {
        return false;
    }
    if (output_is_active(o)) {
        return true;
    }
    return output_attach_auto(o);
}

void output_turn_off(output *o)
{
    output_remember(o);
    output_remove_from_layout(o);
    if (!o->wlr_output->enabled) {
        return;
    }
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, false);
    if (!wlr_output_commit_state(o->wlr_output, &state)) {
        wlr_log(WLR_ERROR, "cannot turn off output %s", o->wlr_output->name);
    }
    wlr_output_state_finish(&state);
}

void output_remember(output *o)
{
    if (output_is_active(o)) {
        o->display.last_enabled = output_current_setting(o);
        o->display.has_last_enabled = true;
    }
}

static bool commit_with_fallback(output *o, const display_setting *setting)
{
    if (commit_setting(o, setting) || commit_setting(o, &automatic_setting)) {
        return true;
    }
    wlr_log(WLR_ERROR, "cannot enable output %s", o->wlr_output->name);
    return false;
}

static bool commit_setting(output *o, const display_setting *setting)
{
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    describe(o->wlr_output, setting, &state);
    bool ok = false;
    if (wants_adaptive_sync(o)) {
        wlr_output_state_set_adaptive_sync_enabled(&state, true);
        ok = wlr_output_commit_state(o->wlr_output, &state);
        wlr_output_state_set_adaptive_sync_enabled(&state, false);
    }
    ok = ok || wlr_output_commit_state(o->wlr_output, &state);
    wlr_output_state_finish(&state);
    return ok;
}

static bool wants_adaptive_sync(const output *o)
{
    return o->server->config.compositing.vblank_mode == WM_VBLANK_ADAPTIVE &&
           o->wlr_output->adaptive_sync_supported;
}

static void describe(struct wlr_output *wlr_output, const display_setting *setting,
                     struct wlr_output_state *state)
{
    wlr_output_state_set_enabled(state, true);
    describe_mode(wlr_output, &setting->mode, state);
    wlr_output_state_set_scale(state, setting->scale > 0.0 ? (float) setting->scale : 1.0f);
    wlr_output_state_set_transform(state, (enum wl_output_transform) setting->transform);
}

static void describe_mode(struct wlr_output *wlr_output, const display_mode *wanted,
                          struct wlr_output_state *state)
{
    struct wlr_output_mode *mode = find_mode(wlr_output, wanted);
    if (mode != NULL) {
        wlr_output_state_set_mode(state, mode);
        return;
    }
    if (wl_list_empty(&wlr_output->modes) && wanted->width > 0 && wanted->height > 0) {
        wlr_output_state_set_custom_mode(state, wanted->width, wanted->height,
                                         wanted->refresh_mhz);
        return;
    }
    mode = wlr_output_preferred_mode(wlr_output);
    if (mode != NULL) {
        wlr_output_state_set_mode(state, mode);
    }
}

static struct wlr_output_mode *find_mode(struct wlr_output *wlr_output,
                                         const display_mode *wanted)
{
    if (wl_list_empty(&wlr_output->modes)) {
        return NULL;
    }
    return mode_at(wlr_output, pick_mode(wlr_output, wanted));
}

static int pick_mode(struct wlr_output *wlr_output, const display_mode *wanted)
{
    size_t count = (size_t) wl_list_length(&wlr_output->modes);
    mode_option *options = calloc(count, sizeof(*options));
    if (options == NULL) {
        return MODE_CHOICE_NONE;
    }
    size_t i = 0;
    struct wlr_output_mode *mode;
    wl_list_for_each(mode, &wlr_output->modes, link) {
        options[i++] = (mode_option){{mode->width, mode->height, mode->refresh}, mode->preferred};
    }
    mode_options list = {options, count};
    int index = mode_choice_pick(&list, wanted);
    free(options);
    return index;
}

static struct wlr_output_mode *mode_at(struct wlr_output *wlr_output, int index)
{
    int i = 0;
    struct wlr_output_mode *mode;
    wl_list_for_each(mode, &wlr_output->modes, link) {
        if (i++ == index) {
            return mode;
        }
    }
    return NULL;
}
