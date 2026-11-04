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

typedef enum color_pass {
    COLOR_PASS_BEST,
    COLOR_PASS_SAFE,
} color_pass;

typedef struct config_request {
    compositor *server;
    const struct wlr_output_configuration_v1 *config;
    backend_step step;
} config_request;

static bool run_with_fallback(const config_request *request);
static bool run_pass(const config_request *request, color_pass pass);
static bool test_step(struct wlr_backend *backend, const pending_states *pending);
static bool commit_step(struct wlr_backend *backend, const pending_states *pending);
static bool prepare(const struct wlr_output_configuration_v1 *config, pending_states *pending,
                    color_pass pass);
static bool render_state(struct wlr_backend_output_state *state,
                         struct wlr_swapchain **swapchain, color_pass pass);
static void describe_color(output *output, struct wlr_output_state *state, color_pass pass);
static void describe_color(output *o, struct wlr_output_state *state, color_pass pass)
{
    if (pass == COLOR_PASS_BEST) {
        output_color_describe(o, state);
    } else {
        wlr_output_state_set_render_format(state, output_color_fallback_format());
    }
}

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
    if (run_pass(request, COLOR_PASS_BEST)) {
        return true;
    }
    bool wants_more = output_color_wanted_format(request->server) !=
                      output_color_fallback_format();
    return wants_more && run_pass(request, COLOR_PASS_SAFE);
}

static bool run_pass(const config_request *request, color_pass pass)
{
    pending_states pending = {0};
    bool ok = prepare(request->config, &pending, pass) &&
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
                    color_pass pass)
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
        if (!render_state(&pending->states[i], &pending->swapchains[i], pass)) {
            return false;
        }
    }
    return true;
}

static bool render_state(struct wlr_backend_output_state *state,
                         struct wlr_swapchain **swapchain, color_pass pass)
{
    output *o = output_from_wlr(state->output);
    if (o == NULL || !will_be_enabled(state)) {
        return true;
    }
    describe_color(o, &state->base, pass);
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
