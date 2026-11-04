#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/render/swapchain.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "output.h"
#include "output_config.h"

typedef struct pending_states {
    struct wlr_backend_output_state *states;
    struct wlr_swapchain **swapchains;
    size_t count;
} pending_states;

static bool prepare(const struct wlr_output_configuration_v1 *config, pending_states *pending);
static bool render_state(struct wlr_backend_output_state *state,
                         struct wlr_swapchain **swapchain);
static bool will_be_enabled(const struct wlr_backend_output_state *state);
static void place_outputs(const struct wlr_output_configuration_v1 *config);
static void finish(pending_states *pending);

bool output_config_test(compositor *server, const struct wlr_output_configuration_v1 *config)
{
    pending_states pending = {0};
    bool ok = prepare(config, &pending) &&
              wlr_backend_test(server->backend, pending.states, pending.count);
    finish(&pending);
    return ok;
}

bool output_config_commit(compositor *server, const struct wlr_output_configuration_v1 *config)
{
    pending_states pending = {0};
    bool ok = prepare(config, &pending) &&
              wlr_backend_test(server->backend, pending.states, pending.count) &&
              wlr_backend_commit(server->backend, pending.states, pending.count);
    finish(&pending);
    if (!ok) {
        wlr_log(WLR_ERROR, "cannot apply the requested output configuration");
        return false;
    }
    place_outputs(config);
    return true;
}

static bool prepare(const struct wlr_output_configuration_v1 *config, pending_states *pending)
{
    pending->states = wlr_output_configuration_v1_build_state(config, &pending->count);
    if (pending->states == NULL) {
        return false;
    }
    pending->swapchains = calloc(pending->count > 0 ? pending->count : 1,
                                 sizeof(pending->swapchains[0]));
    if (pending->swapchains == NULL) {
        return false;
    }
    for (size_t i = 0; i < pending->count; i++) {
        if (!render_state(&pending->states[i], &pending->swapchains[i])) {
            return false;
        }
    }
    return true;
}

static bool render_state(struct wlr_backend_output_state *state,
                         struct wlr_swapchain **swapchain)
{
    output *o = output_from_wlr(state->output);
    if (o == NULL || !will_be_enabled(state)) {
        return true;
    }
    struct wlr_scene_output *scene_output = output_scene(o);
    if (scene_output == NULL ||
        !wlr_output_configure_primary_swapchain(state->output, &state->base, swapchain)) {
        return false;
    }
    struct wlr_scene_output_state_options options = {.swapchain = *swapchain};
    return wlr_scene_output_build_state(scene_output, &state->base, &options);
}

static bool will_be_enabled(const struct wlr_backend_output_state *state)
{
    if ((state->base.committed & WLR_OUTPUT_STATE_ENABLED) != 0) {
        return state->base.enabled;
    }
    return state->output->enabled;
}

static void place_outputs(const struct wlr_output_configuration_v1 *config)
{
    struct wlr_output_configuration_head_v1 *head;
    wl_list_for_each(head, &config->heads, link) {
        output *o = output_from_wlr(head->state.output);
        if (o == NULL) {
            continue;
        }
        if (head->state.enabled) {
            output_place(o, (wm_point){head->state.x, head->state.y});
        } else {
            output_remove_from_layout(o);
        }
    }
}

static void finish(pending_states *pending)
{
    for (size_t i = 0; i < pending->count; i++) {
        wlr_output_state_finish(&pending->states[i].base);
        if (pending->swapchains != NULL && pending->swapchains[i] != NULL) {
            wlr_swapchain_destroy(pending->swapchains[i]);
        }
    }
    free(pending->states);
    free(pending->swapchains);
}
