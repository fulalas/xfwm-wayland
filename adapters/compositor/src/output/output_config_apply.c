#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/render/swapchain.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "output.h"
#include "output_color.h"
#include "output_config.h"

typedef struct pending_states {
    struct wlr_backend_output_state *states;
    struct wlr_swapchain **swapchains;
    size_t count;
} pending_states;

typedef bool (*backend_step)(struct wlr_backend *backend, const pending_states *pending);

typedef void (*color_step)(output *output, struct wlr_output_state *state);

typedef struct config_request {
    compositor *server;
    const struct wlr_output_configuration_v1 *config;
    backend_step step;
} config_request;

static bool run_with_fallback(const config_request *request);
static bool wants_rich_color(const compositor *server);
static bool run_pass(const config_request *request, color_step color);
static bool test_step(struct wlr_backend *backend, const pending_states *pending);
static bool commit_step(struct wlr_backend *backend, const pending_states *pending);
static bool prepare(const struct wlr_output_configuration_v1 *config, pending_states *pending,
                    color_step color);
static bool render_state(struct wlr_backend_output_state *state,
                         struct wlr_swapchain **swapchain, color_step color);
static bool will_be_enabled(const struct wlr_backend_output_state *state);
static void place_outputs(const struct wlr_output_configuration_v1 *config);
static void finish(pending_states *pending);

bool output_config_test(compositor *server, const struct wlr_output_configuration_v1 *config)
{
    config_request request = {server, config, test_step};
    return run_with_fallback(&request);
}

bool output_config_commit(compositor *server, const struct wlr_output_configuration_v1 *config)
{
    config_request request = {server, config, commit_step};
    if (!run_with_fallback(&request)) {
        wlr_log(WLR_ERROR, "cannot apply the requested output configuration");
        return false;
    }
    place_outputs(config);
    return true;
}

static bool run_with_fallback(const config_request *request)
{
    return run_pass(request, output_color_describe) ||
           (wants_rich_color(request->server) &&
            run_pass(request, output_color_describe_basic));
}

static bool wants_rich_color(const compositor *server)
{
    const wm_compositing_config *config = &server->config.compositing;
    return config->hdr || config->ten_bit_color;
}

static bool run_pass(const config_request *request, color_step color)
{
    pending_states pending = {0};
    bool ok = prepare(request->config, &pending, color) &&
              request->step(request->server->backend, &pending);
    finish(&pending);
    return ok;
}

static bool test_step(struct wlr_backend *backend, const pending_states *pending)
{
    return wlr_backend_test(backend, pending->states, pending->count);
}

static bool commit_step(struct wlr_backend *backend, const pending_states *pending)
{
    return test_step(backend, pending) &&
           wlr_backend_commit(backend, pending->states, pending->count);
}

static bool prepare(const struct wlr_output_configuration_v1 *config, pending_states *pending,
                    color_step color)
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
        if (!render_state(&pending->states[i], &pending->swapchains[i], color)) {
            return false;
        }
    }
    return true;
}

static bool render_state(struct wlr_backend_output_state *state,
                         struct wlr_swapchain **swapchain, color_step color)
{
    output *o = output_from_wlr(state->output);
    if (o == NULL || !will_be_enabled(state)) {
        return true;
    }
    color(o, &state->base);
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
