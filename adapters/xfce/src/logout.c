#include "logout.h"

#include <gio/gio.h>

#include "logout_policy.h"
#include "spawn.h"

static const char manager_name[] = "org.xfce.SessionManager";
static const char manager_path[] = "/org/xfce/SessionManager";
static const char manager_interface[] = "org.xfce.Session.Manager";
static const char fallback_command[] = "xfce4-session-logout";

static void on_bus_ready(GObject *source, GAsyncResult *result, gpointer data);
static void on_logout_replied(GObject *source, GAsyncResult *result, gpointer data);
static void handle_call_error(GError *error);

void logout_request(void)
{
    g_bus_get(G_BUS_TYPE_SESSION, NULL, on_bus_ready, NULL);
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
    g_dbus_connection_call(bus, manager_name, manager_path, manager_interface, "Logout",
                           g_variant_new("(bb)", TRUE, TRUE), NULL,
                           G_DBUS_CALL_FLAGS_NO_AUTO_START, -1, NULL, on_logout_replied, NULL);
    g_object_unref(bus);
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
