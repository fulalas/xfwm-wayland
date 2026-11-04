#include "gtk_bridge.h"

#include <glib.h>
#include <stdbool.h>

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
    GHashTable *icon_lines;
    bool helper_restarted;
};

static void on_line(void *self, const char *line);
static void on_stopped(void *self);
static void handle_message(gtk_bridge *bridge, const helper_message *message);
static void finish_style(gtk_bridge *bridge, const helper_message *message);
static void resend_icon_lines(gtk_bridge *bridge);
static void send_icon_line(void *id, void *line, void *self);

gtk_bridge *gtk_bridge_create(const bridge_listener *listener)
{
    gtk_bridge *bridge = g_new0(gtk_bridge, 1);
    helper_events events = { .self = bridge, .line = on_line, .stopped = on_stopped };

    bridge->listener = *listener;
    bridge->client = helper_client_create(&events);
    bridge->style = style_state_create();
    bridge->icons = icon_lookups_create();
    bridge->dialogs = terminate_dialogs_create();
    bridge->icon_lines = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    bridge->helper_restarted = true;
    return bridge;
}

void gtk_bridge_destroy(gtk_bridge *bridge)
{
    g_hash_table_destroy(bridge->icon_lines);
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
    char *line = helper_icon_request_format(id, request);

    g_hash_table_insert(bridge->icon_lines, GUINT_TO_POINTER(id), line);
    if (helper_client_is_running(bridge->client)) {
        helper_client_send(bridge->client, line);
    }
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

    bridge->helper_restarted = true;
}

static void handle_message(gtk_bridge *bridge, const helper_message *message)
{
    if (message->kind == HELPER_MESSAGE_ICON) {
        g_hash_table_remove(bridge->icon_lines, GUINT_TO_POINTER(message->id));
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
    resend_icon_lines(bridge);
    if (changed && bridge->listener.style_changed) {
        bridge->listener.style_changed(bridge->listener.self);
    }
}

static void resend_icon_lines(gtk_bridge *bridge)
{
    if (!bridge->helper_restarted) {
        return;
    }
    bridge->helper_restarted = false;
    g_hash_table_foreach(bridge->icon_lines, send_icon_line, bridge);
}

static void send_icon_line(void *id, void *line, void *self)
{
    gtk_bridge *bridge = self;

    (void) id;
    helper_client_send(bridge->client, line);
}
