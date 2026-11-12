#include "logout.h"

#include <gio/gio.h>

#include "logout_policy.h"
#include "spawn.h"

static const char manager_name[] = "org.xfce.SessionManager";
static const char manager_path[] = "/org/xfce/SessionManager";
static const char manager_interface[] = "org.xfce.Session.Manager";
static const char fallback_command[] = "xfce4-session-logout";

static const guint state_wait_seconds = 15;

static guint state_watch;
static guint state_timeout;

static void on_bus_ready(GObject *source, GAsyncResult *result, gpointer data);
static void on_state_replied(GObject *source, GAsyncResult *result, gpointer data);
static void on_state_changed(GDBusConnection *bus, const gchar *sender, const gchar *path,
                             const gchar *interface, const gchar *signal, GVariant *parameters,
                             gpointer data);
static void finish_waiting(GDBusConnection *bus, guint32 state);
static void fail_waiting(GDBusConnection *bus, GError *error);
static gboolean on_state_timeout(gpointer data);
static void stop_waiting(GDBusConnection *bus);
static void send_logout(GDBusConnection *bus);
static void on_logout_replied(GObject *source, GAsyncResult *result, gpointer data);
static void handle_call_error(GError *error);

void logout_request(void)
{
    if (state_watch == 0) {
        g_bus_get(G_BUS_TYPE_SESSION, NULL, on_bus_ready, NULL);
    }
}

static void on_bus_ready(GObject *source, GAsyncResult *result, gpointer data)
{
    GError *error = NULL;
    GDBusConnection *bus = g_bus_get_finish(result, &error);

    (void) source;
    (void) data;
    if (!bus) {
        g_warning("Cannot reach the session bus to log out: %s", error->message);
        g_error_free(error);
        spawn_command(fallback_command);
        return;
    }
    if (state_watch == 0) {
        state_watch = g_dbus_connection_signal_subscribe(
            bus, manager_name, manager_interface, "StateChanged", manager_path, NULL,
            G_DBUS_SIGNAL_FLAGS_NONE, on_state_changed, g_object_ref(bus), g_object_unref);
        state_timeout = g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, state_wait_seconds,
                                                   on_state_timeout, g_object_ref(bus),
                                                   g_object_unref);
        g_dbus_connection_call(bus, manager_name, manager_path, manager_interface, "GetState",
                               NULL, G_VARIANT_TYPE("(u)"), G_DBUS_CALL_FLAGS_NO_AUTO_START, -1,
                               NULL, on_state_replied, NULL);
    }
    g_object_unref(bus);
}

static void on_state_replied(GObject *source, GAsyncResult *result, gpointer data)
{
    GDBusConnection *bus = G_DBUS_CONNECTION(source);
    GError *error = NULL;
    GVariant *reply = g_dbus_connection_call_finish(bus, result, &error);
    guint32 state;

    (void) data;
    if (!reply) {
        fail_waiting(bus, error);
        g_error_free(error);
        return;
    }
    g_variant_get(reply, "(u)", &state);
    g_variant_unref(reply);
    finish_waiting(bus, state);
}

static void on_state_changed(GDBusConnection *bus, const gchar *sender, const gchar *path,
                             const gchar *interface, const gchar *signal, GVariant *parameters,
                             gpointer data)
{
    guint32 new_state;

    (void) sender;
    (void) path;
    (void) interface;
    (void) signal;
    (void) data;
    if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(uu)"))) {
        return;
    }
    g_variant_get(parameters, "(uu)", NULL, &new_state);
    finish_waiting(bus, new_state);
}

static void finish_waiting(GDBusConnection *bus, guint32 state)
{
    if (state_watch == 0 || logout_waits_for_state(state)) {
        return;
    }
    stop_waiting(bus);
    send_logout(bus);
}

static void fail_waiting(GDBusConnection *bus, GError *error)
{
    if (state_watch == 0) {
        return;
    }
    stop_waiting(bus);
    handle_call_error(error);
}

static gboolean on_state_timeout(gpointer data)
{
    GDBusConnection *bus = data;

    state_timeout = 0;
    stop_waiting(bus);
    send_logout(bus);
    return G_SOURCE_REMOVE;
}

static void stop_waiting(GDBusConnection *bus)
{
    if (state_timeout != 0) {
        g_source_remove(state_timeout);
        state_timeout = 0;
    }
    g_dbus_connection_signal_unsubscribe(bus, state_watch);
    state_watch = 0;
}

static void send_logout(GDBusConnection *bus)
{
    g_dbus_connection_call(bus, manager_name, manager_path, manager_interface, "Logout",
                           g_variant_new("(bb)", TRUE, TRUE), NULL,
                           G_DBUS_CALL_FLAGS_NO_AUTO_START, -1, NULL, on_logout_replied, NULL);
}

static void on_logout_replied(GObject *source, GAsyncResult *result, gpointer data)
{
    GError *error = NULL;
    GVariant *reply = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), result, &error);

    (void) data;
    if (reply) {
        g_variant_unref(reply);
        return;
    }
    handle_call_error(error);
    g_error_free(error);
}

static void handle_call_error(GError *error)
{
    gchar *remote = g_dbus_error_get_remote_error(error);

    if (logout_manager_unavailable(remote)) {
        spawn_command(fallback_command);
    } else {
        g_warning("The session manager refused to log out: %s", error->message);
    }
    g_free(remote);
}
