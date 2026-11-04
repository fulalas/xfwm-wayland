#include <stdlib.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_power_management_v1.h>
#include <wlr/util/log.h>

#include "listener.h"
#include "output.h"
#include "output_config.h"
#include "output_power.h"

struct output_power {
    compositor *server;
    struct wl_listener set_mode;
};

static void handle_set_mode(struct wl_listener *listener, void *data);

output_power *output_power_create(compositor *server)
{
    output_power *p = calloc(1, sizeof(*p));
    if (p == NULL) {
        return NULL;
    }
    p->server = server;
    wl_list_init(&p->set_mode.link);
    if (server->globals.output_power != NULL) {
        listener_connect(&p->set_mode, &server->globals.output_power->events.set_mode,
                         handle_set_mode);
    }
    return p;
}

void output_power_destroy(output_power *p)
{
    listener_disconnect(&p->set_mode);
    free(p);
}

static void handle_set_mode(struct wl_listener *listener, void *data)
{
    output_power *p = wl_container_of(listener, p, set_mode);
    const struct wlr_output_power_v1_set_mode_event *event = data;
    output *o = output_from_wlr(event->output);
    if (o == NULL || !output_is_active(o)) {
        return;
    }
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, event->mode == ZWLR_OUTPUT_POWER_V1_MODE_ON);
    if (!wlr_output_commit_state(event->output, &state)) {
        wlr_log(WLR_ERROR, "cannot change the power mode of output %s", event->output->name);
    }
    wlr_output_state_finish(&state);
    outputs_mark_changed(p->server->outputs);
    outputs_schedule_frames(p->server->outputs);
    output_config_schedule_publish(p->server->output_config);
}
