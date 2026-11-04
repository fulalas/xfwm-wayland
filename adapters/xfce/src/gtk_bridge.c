#include "gtk_bridge.h"

#include <glib.h>

#include "helper_client.h"
#include "helper_protocol.h"
#include "icon_lookups.h"
#include "style_state.h"
#include "terminate.h"

struct gtk_bridge {
    bridge_listener listener;
    helper_client *client;
    style_state *style;
    icon_lookups *icons;
    terminate_dialogs *dialogs;
    GArray *unsent_icons;
    guint fail_icons_id;
};

static void on_line(void *self, const char *line);
static void on_stopped(void *self);
static void handle_message(gtk_bridge *bridge, const helper_message *message);
static void finish_style(gtk_bridge *bridge, const helper_message *message);
static void schedule_icon_failure(gtk_bridge *bridge, unsigned id);
static gboolean on_fail_icons(void *data);

gtk_bridge *gtk_bridge_create(const bridge_listener *listener)
{
    gtk_bridge *bridge = g_new0(gtk_bridge, 1);
    helper_events events = { .self = bridge, .line = on_line, .stopped = on_stopped };

    bridge->listener = *listener;
    bridge->client = helper_client_create(&events);
    bridge->style = style_state_create();
    bridge->icons = icon_lookups_create();
    bridge->dialogs = terminate_dialogs_create();
    bridge->unsent_icons = g_array_new(FALSE, FALSE, sizeof(unsigned));
    return bridge;
}

void gtk_bridge_destroy(gtk_bridge *bridge)
{
    if (bridge->fail_icons_id) {
        g_source_remove(bridge->fail_icons_id);
    }
    g_array_free(bridge->unsent_icons, TRUE);
    helper_client_destroy(bridge->client);
    terminate_dialogs_destroy(bridge->dialogs);
    icon_lookups_destroy(bridge->icons);
    style_state_destroy(bridge->style);
    g_free(bridge);
}

void gtk_bridge_start(gtk_bridge *bridge)
{
    helper_client_start(bridge->client);
}

xfce_style gtk_bridge_style(const gtk_bridge *bridge)
{
    return style_state_current(bridge->style);
}

void gtk_bridge_lookup_icon(gtk_bridge *bridge, const icon_request *request)
{
    unsigned id = icon_lookups_add(bridge->icons, request);
    char *line;

    if (!helper_client_is_running(bridge->client)) {
        schedule_icon_failure(bridge, id);
        return;
    }
    line = helper_icon_request_format(id, request);
    helper_client_send(bridge->client, line);
    g_free(line);
}

void gtk_bridge_cancel_icons(gtk_bridge *bridge, void *context)
{
    icon_lookups_cancel(bridge->icons, context);
}

void gtk_bridge_ask_terminate(gtk_bridge *bridge, const terminate_request *request)
{
    terminate_dialogs_ask(bridge->dialogs, request);
}

static void on_line(void *self, const char *line)
{
    gtk_bridge *bridge = self;
    helper_message *message = g_new(helper_message, 1);

    if (helper_message_parse(line, message)) {
        handle_message(bridge, message);
    } else {
        g_warning("Ignoring unknown GTK helper message: %s", line);
    }
    g_free(message);
}

static void on_stopped(void *self)
{
    gtk_bridge *bridge = self;

    icon_lookups_fail_all(bridge->icons);
}

static void handle_message(gtk_bridge *bridge, const helper_message *message)
{
    if (message->kind == HELPER_MESSAGE_ICON) {
        icon_lookups_finish(bridge->icons, message->id, message->text);
    } else if (message->kind == HELPER_MESSAGE_STYLE_END) {
        finish_style(bridge, message);
    } else {
        style_state_apply(bridge->style, message);
    }
}

static void finish_style(gtk_bridge *bridge, const helper_message *message)
{
    bool changed = style_state_pending_differs(bridge->style);

    style_state_apply(bridge->style, message);
    helper_client_reset_backoff(bridge->client);
    if (changed && bridge->listener.style_changed) {
        bridge->listener.style_changed(bridge->listener.self);
    }
}

static void schedule_icon_failure(gtk_bridge *bridge, unsigned id)
{
    g_array_append_val(bridge->unsent_icons, id);
    if (!bridge->fail_icons_id) {
        bridge->fail_icons_id = g_idle_add(on_fail_icons, bridge);
    }
}

static gboolean on_fail_icons(void *data)
{
    gtk_bridge *bridge = data;
    GArray *unsent = bridge->unsent_icons;

    bridge->fail_icons_id = 0;
    bridge->unsent_icons = g_array_new(FALSE, FALSE, sizeof(unsigned));
    for (guint i = 0; i < unsent->len; i++) {
        icon_lookups_finish(bridge->icons, g_array_index(unsent, unsigned, i), NULL);
    }
    g_array_free(unsent, TRUE);
    return G_SOURCE_REMOVE;
}
