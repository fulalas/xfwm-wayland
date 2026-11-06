#include "power_source.h"

#include <gio/gio.h>

static const char upower_name[] = "org.freedesktop.UPower";
static const char upower_path[] = "/org/freedesktop/UPower";
static const char upower_interface[] = "org.freedesktop.UPower";
static const char on_battery_property[] = "OnBattery";
static const char power_manager_name[] = "org.xfce.PowerManager";

struct power_source {
    power_source_listener listener;
    GCancellable *cancellable;
    GDBusProxy *proxy;
    guint manager_watch;
    bool manager_running;
};

static void on_proxy_ready(GObject *object, GAsyncResult *result, gpointer data);
static void on_properties_changed(GDBusProxy *proxy, GVariant *changed, GStrv invalidated,
                                  gpointer data);
static void on_manager_appeared(GDBusConnection *connection, const char *name, const char *owner,
                                gpointer data);
static void on_manager_vanished(GDBusConnection *connection, const char *name, gpointer data);
static void notify(const power_source *source);

power_source *power_source_create(const power_source_listener *listener)
{
    power_source *source = g_new0(power_source, 1);

    source->listener = *listener;
    source->cancellable = g_cancellable_new();
    g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, NULL, upower_name,
                             upower_path, upower_interface, source->cancellable, on_proxy_ready,
                             source);
    source->manager_watch =
        g_bus_watch_name(G_BUS_TYPE_SESSION, power_manager_name, G_BUS_NAME_WATCHER_FLAGS_NONE,
                         on_manager_appeared, on_manager_vanished, source, NULL);
    return source;
}

void power_source_destroy(power_source *source)
{
    if (source == NULL) {
        return;
    }
    g_bus_unwatch_name(source->manager_watch);
    g_cancellable_cancel(source->cancellable);
    g_object_unref(source->cancellable);
    if (source->proxy != NULL) {
        g_signal_handlers_disconnect_by_data(source->proxy, source);
        g_object_unref(source->proxy);
    }
    g_free(source);
}

bool power_source_on_battery(const power_source *source)
{
    GVariant *value = source->proxy != NULL
                          ? g_dbus_proxy_get_cached_property(source->proxy, on_battery_property)
                          : NULL;
    bool on_battery = value != NULL && g_variant_get_boolean(value);

    if (value != NULL) {
        g_variant_unref(value);
    }
    return on_battery;
}

bool power_source_manager_running(const power_source *source)
{
    return source->manager_running;
}

static void on_proxy_ready(GObject *object, GAsyncResult *result, gpointer data)
{
    (void) object;
    GDBusProxy *proxy = g_dbus_proxy_new_for_bus_finish(result, NULL);

    if (proxy == NULL) {
        return;
    }
    power_source *source = data;
    source->proxy = proxy;
    g_signal_connect(proxy, "g-properties-changed", G_CALLBACK(on_properties_changed), source);
    notify(source);
}

static void on_properties_changed(GDBusProxy *proxy, GVariant *changed, GStrv invalidated,
                                  gpointer data)
{
    (void) proxy;
    (void) changed;
    (void) invalidated;
    notify(data);
}

static void on_manager_appeared(GDBusConnection *connection, const char *name, const char *owner,
                                gpointer data)
{
    (void) connection;
    (void) name;
    (void) owner;
    power_source *source = data;
    source->manager_running = true;
    notify(source);
}

static void on_manager_vanished(GDBusConnection *connection, const char *name, gpointer data)
{
    (void) connection;
    (void) name;
    power_source *source = data;
    source->manager_running = false;
    notify(source);
}

static void notify(const power_source *source)
{
    if (source->listener.changed != NULL) {
        source->listener.changed(source->listener.self);
    }
}
