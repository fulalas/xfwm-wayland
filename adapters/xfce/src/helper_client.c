#include "helper_client.h"

#include <glib.h>

#include "helper_path.h"
#include "helper_session.h"

enum {
    RESTART_DELAY_MIN_MS = 1000,
    RESTART_DELAY_MAX_MS = 60000,
    BACKOFF_FACTOR = 2,
    FIRST_IDLE_STOP_MS = 60000,
    IDLE_STOP_MS = 10000,
};

struct helper_client {
    helper_events events;
    helper_session *session;
    guint restart_id;
    guint idle_id;
    gint64 first_run_end_us;
    unsigned restart_delay_ms;
};

static void spawn(helper_client *client);
static void schedule_restart(helper_client *client);
static gboolean on_restart(void *data);
static void stay_awake(helper_client *client);
static unsigned idle_stop_ms(helper_client *client);
static void cancel_idle(helper_client *client);
static void drop_session(helper_client *client);
static gboolean on_idle(void *data);
static void on_line(void *self, const char *line);
static void on_exited(void *self);

helper_client *helper_client_create(const helper_events *events)
{
    helper_client *client = g_new0(helper_client, 1);

    client->events = *events;
    client->restart_delay_ms = RESTART_DELAY_MIN_MS;
    client->first_run_end_us = g_get_monotonic_time() + (gint64) FIRST_IDLE_STOP_MS * 1000;
    return client;
}

void helper_client_destroy(helper_client *client)
{
    if (client->restart_id) {
        g_source_remove(client->restart_id);
    }
    drop_session(client);
    g_free(client);
}

void helper_client_start(helper_client *client)
{
    if (client->session) {
        stay_awake(client);
    } else if (!client->restart_id) {
        spawn(client);
    }
}

void helper_client_send(helper_client *client, const char *line)
{
    helper_client_start(client);
    if (client->session) {
        helper_session_send(client->session, line);
    }
}

void helper_client_reset_backoff(helper_client *client)
{
    client->restart_delay_ms = RESTART_DELAY_MIN_MS;
}

static void spawn(helper_client *client)
{
    session_events events = { .self = client, .line = on_line, .exited = on_exited };
    char *path = helper_path_find();

    if (path) {
        client->session = helper_session_spawn(path, &events);
    } else {
        g_warning("Cannot find the GTK helper xfwm-wayland-helper");
    }
    g_free(path);
    if (client->session) {
        stay_awake(client);
    } else {
        schedule_restart(client);
    }
}

static void schedule_restart(helper_client *client)
{
    client->restart_id = g_timeout_add(client->restart_delay_ms, on_restart, client);
    client->restart_delay_ms = MIN(client->restart_delay_ms * BACKOFF_FACTOR,
                                   RESTART_DELAY_MAX_MS);
}

static gboolean on_restart(void *data)
{
    helper_client *client = data;

    client->restart_id = 0;
    spawn(client);
    return G_SOURCE_REMOVE;
}

static unsigned idle_stop_ms(helper_client *client)
{
    gint64 left_us = client->first_run_end_us - g_get_monotonic_time();
    gint64 left_ms = (left_us + 999) / 1000;

    return (unsigned) MAX(left_ms, IDLE_STOP_MS);
}

static void stay_awake(helper_client *client)
{
    cancel_idle(client);
    client->idle_id = g_timeout_add(idle_stop_ms(client), on_idle, client);
}

static void cancel_idle(helper_client *client)
{
    if (client->idle_id) {
        g_source_remove(client->idle_id);
        client->idle_id = 0;
    }
}

static void drop_session(helper_client *client)
{
    cancel_idle(client);
    if (client->session) {
        helper_session_destroy(client->session);
        client->session = NULL;
    }
}

static gboolean on_idle(void *data)
{
    helper_client *client = data;

    client->idle_id = 0;
    drop_session(client);
    client->events.stopped(client->events.self);
    return G_SOURCE_REMOVE;
}

static void on_line(void *self, const char *line)
{
    helper_client *client = self;

    stay_awake(client);
    client->events.line(client->events.self, line);
}

static void on_exited(void *self)
{
    helper_client *client = self;

    g_warning("The GTK helper exited, restarting it in %u ms", client->restart_delay_ms);
    drop_session(client);
    schedule_restart(client);
    client->events.stopped(client->events.self);
}
