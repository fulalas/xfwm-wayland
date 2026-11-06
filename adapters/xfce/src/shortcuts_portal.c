#include "shortcuts_portal.h"

#include <gio/gio.h>

#include "app_shortcuts.h"

static const char bus_name[] = "org.freedesktop.impl.portal.desktop.xfwm";
static const char object_path[] = "/org/freedesktop/portal/desktop";
static const char shortcuts_interface[] = "org.freedesktop.impl.portal.GlobalShortcuts";
static const char session_interface[] = "org.freedesktop.impl.portal.Session";

enum {
    PORTAL_VERSION = 1,
    RESPONSE_SUCCESS = 0,
};

static const char introspection[] =
    "<node>"
    "  <interface name='org.freedesktop.impl.portal.GlobalShortcuts'>"
    "    <method name='CreateSession'>"
    "      <arg type='o' name='handle' direction='in'/>"
    "      <arg type='o' name='session_handle' direction='in'/>"
    "      <arg type='s' name='app_id' direction='in'/>"
    "      <arg type='a{sv}' name='options' direction='in'/>"
    "      <arg type='u' name='response' direction='out'/>"
    "      <arg type='a{sv}' name='results' direction='out'/>"
    "    </method>"
    "    <method name='BindShortcuts'>"
    "      <arg type='o' name='handle' direction='in'/>"
    "      <arg type='o' name='session_handle' direction='in'/>"
    "      <arg type='a(sa{sv})' name='shortcuts' direction='in'/>"
    "      <arg type='s' name='parent_window' direction='in'/>"
    "      <arg type='a{sv}' name='options' direction='in'/>"
    "      <arg type='u' name='response' direction='out'/>"
    "      <arg type='a{sv}' name='results' direction='out'/>"
    "    </method>"
    "    <method name='ListShortcuts'>"
    "      <arg type='o' name='handle' direction='in'/>"
    "      <arg type='o' name='session_handle' direction='in'/>"
    "      <arg type='u' name='response' direction='out'/>"
    "      <arg type='a{sv}' name='results' direction='out'/>"
    "    </method>"
    "    <signal name='Activated'>"
    "      <arg type='o' name='session_handle'/>"
    "      <arg type='s' name='shortcut_id'/>"
    "      <arg type='t' name='timestamp'/>"
    "      <arg type='a{sv}' name='options'/>"
    "    </signal>"
    "    <signal name='Deactivated'>"
    "      <arg type='o' name='session_handle'/>"
    "      <arg type='s' name='shortcut_id'/>"
    "      <arg type='t' name='timestamp'/>"
    "      <arg type='a{sv}' name='options'/>"
    "    </signal>"
    "    <property name='version' type='u' access='read'/>"
    "  </interface>"
    "  <interface name='org.freedesktop.impl.portal.Session'>"
    "    <method name='Close'/>"
    "    <property name='version' type='u' access='read'/>"
    "  </interface>"
    "</node>";

typedef struct portal_session {
    guint registration;
    char *owner;
} portal_session;

struct shortcuts_portal {
    shortcuts_portal_listener listener;
    app_shortcuts *shortcuts;
    GDBusNodeInfo *node;
    GDBusConnection *connection;
    GHashTable *sessions;
    GHashTable *owner_watches;
    guint owner;
    guint registration;
};

static void on_bus_acquired(GDBusConnection *connection, const char *name, gpointer data);
static void on_method(GDBusConnection *connection, const char *sender, const char *path,
                      const char *interface, const char *method, GVariant *parameters,
                      GDBusMethodInvocation *invocation, gpointer data);
static GVariant *on_get_property(GDBusConnection *connection, const char *sender,
                                 const char *path, const char *interface, const char *property,
                                 GError **error, gpointer data);
static void on_session_method(GDBusConnection *connection, const char *sender, const char *path,
                              const char *interface, const char *method, GVariant *parameters,
                              GDBusMethodInvocation *invocation, gpointer data);
static void create_session(shortcuts_portal *portal, GVariant *parameters,
                           GDBusMethodInvocation *invocation);
static void bind_shortcuts(shortcuts_portal *portal, GVariant *parameters,
                           GDBusMethodInvocation *invocation);
static void list_shortcuts(shortcuts_portal *portal, GVariant *parameters,
                           GDBusMethodInvocation *invocation);
static void return_results(GDBusMethodInvocation *invocation, GVariant *results);
static void return_shortcuts(GDBusMethodInvocation *invocation, const shortcuts_portal *portal,
                             const char *session);
static GVariant *empty_vardict(void);
static app_shortcut_request request_from(GVariant *shortcut);
static GVariant *shortcuts_of(const shortcuts_portal *portal, const char *session);
static bool owns_session(const shortcuts_portal *portal, GDBusMethodInvocation *invocation,
                         const char *session);
static void watch_owner(shortcuts_portal *portal, const char *owner);
static void on_owner_vanished(GDBusConnection *connection, const char *name, gpointer data);
static void close_session(shortcuts_portal *portal, const char *session);
static void session_free(gpointer data);
static void unwatch(gpointer data);
static void notify_changed(shortcuts_portal *portal);

static const GDBusInterfaceVTable portal_vtable = { on_method, on_get_property, NULL, { 0 } };
static const GDBusInterfaceVTable session_vtable = { on_session_method, on_get_property, NULL,
                                                     { 0 } };

shortcuts_portal *shortcuts_portal_create(const shortcuts_portal_listener *listener)
{
    shortcuts_portal *portal = g_new0(shortcuts_portal, 1);

    portal->listener = *listener;
    portal->shortcuts = app_shortcuts_create();
    portal->node = g_dbus_node_info_new_for_xml(introspection, NULL);
    portal->sessions = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, session_free);
    portal->owner_watches = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, unwatch);
    portal->owner = g_bus_own_name(G_BUS_TYPE_SESSION, bus_name, G_BUS_NAME_OWNER_FLAGS_NONE,
                                   on_bus_acquired, NULL, NULL, portal, NULL);
    return portal;
}

void shortcuts_portal_destroy(shortcuts_portal *portal)
{
    if (portal == NULL) {
        return;
    }
    g_bus_unown_name(portal->owner);
    GHashTableIter sessions;
    gpointer value;
    g_hash_table_iter_init(&sessions, portal->sessions);
    while (g_hash_table_iter_next(&sessions, NULL, &value)) {
        const portal_session *session = value;
        g_dbus_connection_unregister_object(portal->connection, session->registration);
    }
    g_hash_table_destroy(portal->sessions);
    g_hash_table_destroy(portal->owner_watches);
    if (portal->connection != NULL && portal->registration != 0) {
        g_dbus_connection_unregister_object(portal->connection, portal->registration);
    }
    g_clear_object(&portal->connection);
    g_dbus_node_info_unref(portal->node);
    app_shortcuts_destroy(portal->shortcuts);
    g_free(portal);
}

size_t shortcuts_portal_keymap(const shortcuts_portal *portal, const wm_app_shortcut **keymap)
{
    return app_shortcuts_keymap(portal->shortcuts, keymap);
}

void shortcuts_portal_activate(shortcuts_portal *portal, uint32_t id, bool pressed,
                               uint32_t time_ms)
{
    const app_shortcut_entry *entry = app_shortcuts_find(portal->shortcuts, id);

    if (entry == NULL || portal->connection == NULL) {
        return;
    }
    GVariant *signal = g_variant_new("(ost@a{sv})", entry->session, entry->shortcut_id,
                                     (guint64) time_ms, empty_vardict());
    g_dbus_connection_emit_signal(portal->connection, NULL, object_path, shortcuts_interface,
                                  pressed ? "Activated" : "Deactivated", signal, NULL);
}

static void on_bus_acquired(GDBusConnection *connection, const char *name, gpointer data)
{
    (void) name;
    shortcuts_portal *portal = data;
    GDBusInterfaceInfo *info = g_dbus_node_info_lookup_interface(portal->node,
                                                                 shortcuts_interface);

    portal->connection = g_object_ref(connection);
    portal->registration = g_dbus_connection_register_object(connection, object_path, info,
                                                             &portal_vtable, portal, NULL, NULL);
}

static void on_method(GDBusConnection *connection, const char *sender, const char *path,
                      const char *interface, const char *method, GVariant *parameters,
                      GDBusMethodInvocation *invocation, gpointer data)
{
    (void) connection;
    (void) sender;
    (void) path;
    (void) interface;
    shortcuts_portal *portal = data;

    if (g_strcmp0(method, "CreateSession") == 0) {
        create_session(portal, parameters, invocation);
    } else if (g_strcmp0(method, "BindShortcuts") == 0) {
        bind_shortcuts(portal, parameters, invocation);
    } else if (g_strcmp0(method, "ListShortcuts") == 0) {
        list_shortcuts(portal, parameters, invocation);
    } else {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown method");
    }
}

static GVariant *on_get_property(GDBusConnection *connection, const char *sender,
                                 const char *path, const char *interface, const char *property,
                                 GError **error, gpointer data)
{
    (void) connection;
    (void) sender;
    (void) path;
    (void) interface;
    (void) error;
    (void) data;
    return g_strcmp0(property, "version") == 0 ? g_variant_new_uint32(PORTAL_VERSION) : NULL;
}

static void on_session_method(GDBusConnection *connection, const char *sender, const char *path,
                              const char *interface, const char *method, GVariant *parameters,
                              GDBusMethodInvocation *invocation, gpointer data)
{
    (void) connection;
    (void) sender;
    (void) interface;
    (void) parameters;
    shortcuts_portal *portal = data;

    if (g_strcmp0(method, "Close") == 0 && owns_session(portal, invocation, path)) {
        close_session(portal, path);
    }
    g_dbus_method_invocation_return_value(invocation, NULL);
}

static void create_session(shortcuts_portal *portal, GVariant *parameters,
                           GDBusMethodInvocation *invocation)
{
    const char *session = NULL;
    GError *error = NULL;
    GDBusInterfaceInfo *info = g_dbus_node_info_lookup_interface(portal->node, session_interface);

    g_variant_get(parameters, "(&o&o&s@a{sv})", NULL, &session, NULL, NULL);
    guint registration = g_dbus_connection_register_object(portal->connection, session, info,
                                                           &session_vtable, portal, NULL, &error);
    if (registration == 0) {
        g_dbus_method_invocation_take_error(invocation, error);
        return;
    }
    portal_session *entry = g_new0(portal_session, 1);
    entry->registration = registration;
    entry->owner = g_strdup(g_dbus_method_invocation_get_sender(invocation));
    g_hash_table_replace(portal->sessions, g_strdup(session), entry);
    watch_owner(portal, entry->owner);
    return_results(invocation, empty_vardict());
}

static void bind_shortcuts(shortcuts_portal *portal, GVariant *parameters,
                           GDBusMethodInvocation *invocation)
{
    const char *session = NULL;
    GVariant *shortcuts = NULL;

    g_variant_get(parameters, "(&o&o@a(sa{sv})&s@a{sv})", NULL, &session, &shortcuts, NULL,
                  NULL);
    if (!owns_session(portal, invocation, session)) {
        g_variant_unref(shortcuts);
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_ACCESS_DENIED, "Unknown session");
        return;
    }
    gsize count = g_variant_n_children(shortcuts);
    app_shortcut_request *requests = g_new0(app_shortcut_request, count);
    GVariant **children = g_new0(GVariant *, count);
    for (gsize i = 0; i < count; i++) {
        children[i] = g_variant_get_child_value(shortcuts, i);
        requests[i] = request_from(children[i]);
    }
    app_shortcuts_bind(portal->shortcuts, session, requests, count);
    for (gsize i = 0; i < count; i++) {
        g_variant_unref(children[i]);
    }
    g_free(children);
    g_free(requests);
    g_variant_unref(shortcuts);
    notify_changed(portal);
    return_shortcuts(invocation, portal, session);
}

static void list_shortcuts(shortcuts_portal *portal, GVariant *parameters,
                           GDBusMethodInvocation *invocation)
{
    const char *session = NULL;

    g_variant_get(parameters, "(&o&o)", NULL, &session);
    if (!owns_session(portal, invocation, session)) {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_ACCESS_DENIED, "Unknown session");
        return;
    }
    return_shortcuts(invocation, portal, session);
}

static void return_results(GDBusMethodInvocation *invocation, GVariant *results)
{
    g_dbus_method_invocation_return_value(invocation,
                                          g_variant_new("(u@a{sv})", RESPONSE_SUCCESS, results));
}

static void return_shortcuts(GDBusMethodInvocation *invocation, const shortcuts_portal *portal,
                             const char *session)
{
    GVariantBuilder results;

    g_variant_builder_init(&results, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&results, "{sv}", "shortcuts", shortcuts_of(portal, session));
    return_results(invocation, g_variant_builder_end(&results));
}

static GVariant *empty_vardict(void)
{
    return g_variant_new_array(G_VARIANT_TYPE("{sv}"), NULL, 0);
}

static app_shortcut_request request_from(GVariant *shortcut)
{
    const char *id = NULL;
    GVariant *properties = NULL;
    app_shortcut_request request = { 0 };

    g_variant_get(shortcut, "(&s@a{sv})", &id, &properties);
    request.id = id;
    g_variant_lookup(properties, "description", "&s", &request.description);
    g_variant_lookup(properties, "preferred_trigger", "&s", &request.trigger);
    g_variant_unref(properties);
    return request;
}

static GVariant *shortcuts_of(const shortcuts_portal *portal, const char *session)
{
    GVariantBuilder list;

    g_variant_builder_init(&list, G_VARIANT_TYPE("a(sa{sv})"));
    for (size_t i = 0; i < app_shortcuts_count(portal->shortcuts); i++) {
        const app_shortcut_entry *entry = app_shortcuts_at(portal->shortcuts, i);
        if (g_strcmp0(entry->session, session) != 0) {
            continue;
        }
        GVariantBuilder properties;
        g_variant_builder_init(&properties, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&properties, "{sv}", "description",
                              g_variant_new_string(entry->description));
        g_variant_builder_add(&properties, "{sv}", "trigger_description",
                              g_variant_new_string(entry->bound ? entry->trigger : ""));
        g_variant_builder_add(&list, "(s@a{sv})", entry->shortcut_id,
                              g_variant_builder_end(&properties));
    }
    return g_variant_builder_end(&list);
}

static void close_session(shortcuts_portal *portal, const char *session)
{
    bool dropped = app_shortcuts_drop_session(portal->shortcuts, session);
    const portal_session *entry = g_hash_table_lookup(portal->sessions, session);

    if (entry != NULL) {
        g_dbus_connection_unregister_object(portal->connection, entry->registration);
        g_hash_table_remove(portal->sessions, session);
    }
    if (dropped) {
        notify_changed(portal);
    }
}

static bool owns_session(const shortcuts_portal *portal, GDBusMethodInvocation *invocation,
                         const char *session)
{
    const portal_session *entry = g_hash_table_lookup(portal->sessions, session);

    return entry != NULL
           && g_strcmp0(entry->owner, g_dbus_method_invocation_get_sender(invocation)) == 0;
}

static void watch_owner(shortcuts_portal *portal, const char *owner)
{
    if (owner == NULL || g_hash_table_contains(portal->owner_watches, owner)) {
        return;
    }
    guint watch = g_bus_watch_name_on_connection(portal->connection, owner,
                                                 G_BUS_NAME_WATCHER_FLAGS_NONE, NULL,
                                                 on_owner_vanished, portal, NULL);
    g_hash_table_insert(portal->owner_watches, g_strdup(owner), GUINT_TO_POINTER(watch));
}

static void on_owner_vanished(GDBusConnection *connection, const char *name, gpointer data)
{
    (void) connection;
    shortcuts_portal *portal = data;
    GPtrArray *orphans = g_ptr_array_new_with_free_func(g_free);
    GHashTableIter sessions;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&sessions, portal->sessions);
    while (g_hash_table_iter_next(&sessions, &key, &value)) {
        if (g_strcmp0(((const portal_session *) value)->owner, name) == 0) {
            g_ptr_array_add(orphans, g_strdup(key));
        }
    }
    for (guint i = 0; i < orphans->len; i++) {
        close_session(portal, g_ptr_array_index(orphans, i));
    }
    g_ptr_array_unref(orphans);
    g_hash_table_remove(portal->owner_watches, name);
}

static void session_free(gpointer data)
{
    portal_session *session = data;

    g_free(session->owner);
    g_free(session);
}

static void unwatch(gpointer data)
{
    g_bus_unwatch_name(GPOINTER_TO_UINT(data));
}

static void notify_changed(shortcuts_portal *portal)
{
    if (portal->listener.changed != NULL) {
        portal->listener.changed(portal->listener.self);
    }
}
