#include "screensaver_service.h"

#include <gio/gio.h>

static const char bus_name[] = "org.freedesktop.ScreenSaver";
static const char interface_name[] = "org.freedesktop.ScreenSaver";
static const char *const object_paths[] = { "/org/freedesktop/ScreenSaver", "/ScreenSaver" };

enum {
    OBJECT_PATH_COUNT = 2,
};

static const char introspection[] =
    "<node>"
    "  <interface name='org.freedesktop.ScreenSaver'>"
    "    <method name='Inhibit'>"
    "      <arg type='s' name='application_name' direction='in'/>"
    "      <arg type='s' name='reason' direction='in'/>"
    "      <arg type='u' name='cookie' direction='out'/>"
    "    </method>"
    "    <method name='UnInhibit'>"
    "      <arg type='u' name='cookie' direction='in'/>"
    "    </method>"
    "    <method name='SimulateUserActivity'/>"
    "    <method name='GetActive'>"
    "      <arg type='b' name='active' direction='out'/>"
    "    </method>"
    "    <method name='SetActive'>"
    "      <arg type='b' name='active' direction='in'/>"
    "      <arg type='b' name='changed' direction='out'/>"
    "    </method>"
    "  </interface>"
    "</node>";

struct screensaver_service {
    screensaver_listener listener;
    GDBusNodeInfo *node;
    GDBusConnection *connection;
    guint owner;
    guint registrations[OBJECT_PATH_COUNT];
    GHashTable *cookies;
    GHashTable *watches;
    guint next_cookie;
};

static void on_bus_acquired(GDBusConnection *connection, const char *name, gpointer data);
static void on_name_lost(GDBusConnection *connection, const char *name, gpointer data);
static void on_method(GDBusConnection *connection, const char *sender, const char *path,
                      const char *interface, const char *method, GVariant *parameters,
                      GDBusMethodInvocation *invocation, gpointer data);
static guint inhibit(screensaver_service *service, const char *sender);
static void uninhibit(screensaver_service *service, guint cookie);
static void watch_sender(screensaver_service *service, const char *sender);
static void on_sender_vanished(GDBusConnection *connection, const char *name, gpointer data);
static void unregister_objects(screensaver_service *service);
static void set_cookies_cleared(screensaver_service *service);
static void notify_if_changed(screensaver_service *service, bool was_inhibited);
static void unwatch(gpointer data);

static const GDBusInterfaceVTable vtable = { on_method, NULL, NULL, { 0 } };

screensaver_service *screensaver_service_create(const screensaver_listener *listener)
{
    screensaver_service *service = g_new0(screensaver_service, 1);

    service->listener = *listener;
    service->node = g_dbus_node_info_new_for_xml(introspection, NULL);
    service->cookies = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    service->watches = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, unwatch);
    service->owner = g_bus_own_name(G_BUS_TYPE_SESSION, bus_name,
                                    G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT
                                        | G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE,
                                    on_bus_acquired, NULL, on_name_lost, service, NULL);
    return service;
}

void screensaver_service_destroy(screensaver_service *service)
{
    if (service == NULL) {
        return;
    }
    g_bus_unown_name(service->owner);
    unregister_objects(service);
    g_hash_table_destroy(service->cookies);
    g_hash_table_destroy(service->watches);
    g_clear_object(&service->connection);
    g_dbus_node_info_unref(service->node);
    g_free(service);
}

bool screensaver_service_inhibited(const screensaver_service *service)
{
    return g_hash_table_size(service->cookies) > 0;
}

static void on_bus_acquired(GDBusConnection *connection, const char *name, gpointer data)
{
    (void) name;
    screensaver_service *service = data;
    GDBusInterfaceInfo *info = g_dbus_node_info_lookup_interface(service->node, interface_name);

    g_set_object(&service->connection, connection);
    for (int i = 0; i < OBJECT_PATH_COUNT; i++) {
        service->registrations[i] = g_dbus_connection_register_object(
            connection, object_paths[i], info, &vtable, service, NULL, NULL);
    }
}

static void on_name_lost(GDBusConnection *connection, const char *name, gpointer data)
{
    (void) connection;
    (void) name;
    screensaver_service *service = data;

    unregister_objects(service);
    set_cookies_cleared(service);
}

static void on_method(GDBusConnection *connection, const char *sender, const char *path,
                      const char *interface, const char *method, GVariant *parameters,
                      GDBusMethodInvocation *invocation, gpointer data)
{
    (void) connection;
    (void) path;
    (void) interface;
    screensaver_service *service = data;

    if (g_strcmp0(method, "Inhibit") == 0) {
        g_dbus_method_invocation_return_value(invocation,
                                              g_variant_new("(u)", inhibit(service, sender)));
    } else if (g_strcmp0(method, "UnInhibit") == 0) {
        guint cookie = 0;
        g_variant_get(parameters, "(u)", &cookie);
        uninhibit(service, cookie);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method, "SimulateUserActivity") == 0) {
        if (service->listener.activity != NULL) {
            service->listener.activity(service->listener.self);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else {
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
    }
}

static guint inhibit(screensaver_service *service, const char *sender)
{
    bool was_inhibited = screensaver_service_inhibited(service);
    guint cookie = ++service->next_cookie;

    g_hash_table_insert(service->cookies, GUINT_TO_POINTER(cookie), g_strdup(sender));
    watch_sender(service, sender);
    notify_if_changed(service, was_inhibited);
    return cookie;
}

static void uninhibit(screensaver_service *service, guint cookie)
{
    bool was_inhibited = screensaver_service_inhibited(service);

    g_hash_table_remove(service->cookies, GUINT_TO_POINTER(cookie));
    notify_if_changed(service, was_inhibited);
}

static void watch_sender(screensaver_service *service, const char *sender)
{
    if (sender == NULL || g_hash_table_contains(service->watches, sender)) {
        return;
    }
    guint watch = g_bus_watch_name_on_connection(service->connection, sender,
                                                 G_BUS_NAME_WATCHER_FLAGS_NONE, NULL,
                                                 on_sender_vanished, service, NULL);
    g_hash_table_insert(service->watches, g_strdup(sender), GUINT_TO_POINTER(watch));
}

static void on_sender_vanished(GDBusConnection *connection, const char *name, gpointer data)
{
    (void) connection;
    screensaver_service *service = data;
    bool was_inhibited = screensaver_service_inhibited(service);
    GHashTableIter cookies;
    gpointer sender;

    g_hash_table_iter_init(&cookies, service->cookies);
    while (g_hash_table_iter_next(&cookies, NULL, &sender)) {
        if (g_strcmp0(sender, name) == 0) {
            g_hash_table_iter_remove(&cookies);
        }
    }
    g_hash_table_remove(service->watches, name);
    notify_if_changed(service, was_inhibited);
}

static void unregister_objects(screensaver_service *service)
{
    for (int i = 0; i < OBJECT_PATH_COUNT; i++) {
        if (service->connection != NULL && service->registrations[i] != 0) {
            g_dbus_connection_unregister_object(service->connection, service->registrations[i]);
        }
        service->registrations[i] = 0;
    }
}

static void set_cookies_cleared(screensaver_service *service)
{
    bool was_inhibited = screensaver_service_inhibited(service);

    g_hash_table_remove_all(service->cookies);
    g_hash_table_remove_all(service->watches);
    notify_if_changed(service, was_inhibited);
}

static void notify_if_changed(screensaver_service *service, bool was_inhibited)
{
    if (was_inhibited != screensaver_service_inhibited(service)
        && service->listener.inhibit_changed != NULL) {
        service->listener.inhibit_changed(service->listener.self);
    }
}

static void unwatch(gpointer data)
{
    g_bus_unwatch_name(GPOINTER_TO_UINT(data));
}
