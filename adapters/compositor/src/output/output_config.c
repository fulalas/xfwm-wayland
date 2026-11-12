#include <stdlib.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/util/log.h>

#include "displays.h"
#include "util/listener.h"
#include "output.h"
#include "output_config.h"
#include "xfce_output_heads.h"

struct output_config {
    compositor *server;
    struct wl_event_source *publish_idle;
    struct wl_listener apply;
    struct wl_listener test;
    struct wl_listener layout_change;
};

static void handle_apply(struct wl_listener *listener, void *data);
static void handle_test(struct wl_listener *listener, void *data);
static void handle_layout_change(struct wl_listener *listener, void *data);
static void handle_publish_idle(void *data);
static void answer(struct wlr_output_configuration_v1 *config, bool succeeded);
static void publish(output_config *config);
static void describe_output(struct wlr_output_configuration_v1 *config, output *output);

output_config *output_config_create(compositor *server)
{
    output_config *c = calloc(1, sizeof(*c));
    if (c == NULL) {
        return NULL;
    }
    c->server = server;
    struct wlr_output_manager_v1 *manager = server->globals.output_manager;
    listener_connect(&c->apply, &manager->events.apply, handle_apply);
    listener_connect(&c->test, &manager->events.test, handle_test);
    listener_connect(&c->layout_change, &server->layout->events.change, handle_layout_change);
    return c;
}

void output_config_destroy(output_config *c)
{
    listener_disconnect(&c->apply);
    listener_disconnect(&c->test);
    listener_disconnect(&c->layout_change);
    if (c->publish_idle != NULL) {
        wl_event_source_remove(c->publish_idle);
    }
    free(c);
}

void output_config_schedule_publish(output_config *c)
{
    if (c != NULL && c->publish_idle == NULL) {
        c->publish_idle = wl_event_loop_add_idle(c->server->loop, handle_publish_idle, c);
    }
}

static void handle_apply(struct wl_listener *listener, void *data)
{
    output_config *c = wl_container_of(listener, c, apply);
    struct wlr_output_configuration_v1 *config = data;
    struct wl_client *client = config->resource != NULL ? wl_resource_get_client(config->resource)
                                                        : NULL;
    bool committed = output_config_commit(c->server, config);
    answer(config, committed);
    if (committed) {
        displays_configured(c->server->displays, client);
    }
    outputs_mark_changed(c->server->outputs);
    outputs_schedule_frames(c->server->outputs);
    output_config_schedule_publish(c);
}

static void handle_test(struct wl_listener *listener, void *data)
{
    output_config *c = wl_container_of(listener, c, test);
    struct wlr_output_configuration_v1 *config = data;
    answer(config, output_config_test(c->server, config));
}

static void handle_layout_change(struct wl_listener *listener, void *data)
{
    (void) data;
    output_config *c = wl_container_of(listener, c, layout_change);
    output_config_schedule_publish(c);
}

static void handle_publish_idle(void *data)
{
    output_config *c = data;
    c->publish_idle = NULL;
    publish(c);
}

static void answer(struct wlr_output_configuration_v1 *config, bool succeeded)
{
    if (succeeded) {
        wlr_output_configuration_v1_send_succeeded(config);
    } else {
        wlr_output_configuration_v1_send_failed(config);
    }
    wlr_output_configuration_v1_destroy(config);
}

static void publish(output_config *c)
{
    compositor *server = c->server;
    if (server->outputs == NULL) {
        return;
    }
    struct wlr_output_configuration_v1 *config = wlr_output_configuration_v1_create();
    if (config == NULL) {
        wlr_log(WLR_ERROR, "cannot describe the output configuration");
        return;
    }
    for (size_t i = 0; i < outputs_known_count(server->outputs); i++) {
        describe_output(config, outputs_known_at(server->outputs, i));
    }
    wlr_output_manager_v1_set_configuration(server->globals.output_manager, config);
    xfce_output_heads_sync(server->xfce_output_heads);
}

static void describe_output(struct wlr_output_configuration_v1 *config, output *o)
{
    struct wlr_output_configuration_head_v1 *head =
        wlr_output_configuration_head_v1_create(config, output_wlr(o));
    if (head == NULL) {
        return;
    }
    head->state.enabled = output_is_active(o);
    wm_rect full = output_full(o);
    head->state.x = full.x;
    head->state.y = full.y;
}
