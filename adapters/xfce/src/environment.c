#include "xfce/environment.h"

#include <glib.h>

#include "channels.h"
#include "dispatcher.h"
#include "gtk_bridge.h"
#include "input_prefs.h"
#include "session_export.h"
#include "settings.h"
#include "shortcuts.h"
#include "spawn.h"

static const char logout_command[] = "xfce4-session-logout";

struct xfce_environment {
    xfce_listener listener;
    channels *channels;
    dispatcher *dispatcher;
    keymap_store *keymap;
    gtk_bridge *bridge;
    char *theme_name;
    char *title_font;
};

static void refresh_settings(xfce_environment *environment);
static void reload_keymap(xfce_environment *environment);
static void on_channel_changed(void *self, channel_id id, const char *property);
static void on_dispatch(void *self, unsigned flags);
static void notify_settings(xfce_environment *environment, unsigned flags);
static void on_style_changed(void *self);
static void notify(xfce_environment *environment, void (*callback)(void *self));
static keyboard_prefs keyboard_prefs_of(void *self);
static pointer_prefs pointer_prefs_of(void *self, const char *device_name);
static cursor_prefs cursor_prefs_of(void *self);
static void lookup_icon(void *self, const icon_request *request);
static void cancel_icon_lookups(void *self, void *context);
static void ask_terminate(void *self, const terminate_request *request);

xfce_environment *xfce_environment_create(const xfce_listener *listener)
{
    xfce_environment *environment = g_new0(xfce_environment, 1);
    channel_watch watch = { .self = environment, .changed = on_channel_changed };
    dispatch_target target = { .self = environment, .run = on_dispatch };
    bridge_listener bridge_listener = { .self = environment, .style_changed = on_style_changed };

    environment->listener = *listener;
    environment->channels = channels_create(&watch);
    environment->dispatcher = dispatcher_create(&target);
    environment->bridge = gtk_bridge_create(&bridge_listener);
    refresh_settings(environment);
    reload_keymap(environment);
    return environment;
}

void xfce_environment_destroy(xfce_environment *environment)
{
    gtk_bridge_destroy(environment->bridge);
    dispatcher_destroy(environment->dispatcher);
    channels_destroy(environment->channels);
    keymap_store_destroy(environment->keymap);
    g_free(environment->theme_name);
    g_free(environment->title_font);
    g_free(environment);
}

void xfce_environment_start_helper(xfce_environment *environment)
{
    gtk_bridge_start(environment->bridge);
}

const char *xfce_environment_theme_name(xfce_environment *environment)
{
    return environment->theme_name;
}

const char *xfce_environment_title_font(xfce_environment *environment)
{
    return environment->title_font;
}

wm_config xfce_environment_config(xfce_environment *environment,
                                  const xfce_setting_source *theme)
{
    return settings_load(environment->channels, theme);
}

wm_keymap xfce_environment_keymap(xfce_environment *environment)
{
    return keymap_store_view(environment->keymap);
}

xfce_style xfce_environment_style(xfce_environment *environment)
{
    return gtk_bridge_style(environment->bridge);
}

compositor_services xfce_environment_services(xfce_environment *environment)
{
    return (compositor_services) {
        .self = environment,
        .keyboard_prefs = keyboard_prefs_of,
        .pointer_prefs = pointer_prefs_of,
        .cursor_prefs = cursor_prefs_of,
        .lookup_icon = lookup_icon,
        .cancel_icon_lookups = cancel_icon_lookups,
        .ask_terminate = ask_terminate,
    };
}

void xfce_environment_store_workspace_count(xfce_environment *environment, int count)
{
    settings_store_workspace_count(channels_get(environment->channels, CHANNEL_XFWM4), count);
}

void xfce_environment_spawn(xfce_environment *environment, const char *command)
{
    (void) environment;
    spawn_command(command);
}

void xfce_environment_logout(xfce_environment *environment)
{
    xfce_environment_spawn(environment, logout_command);
}

void xfce_environment_export_session(xfce_environment *environment,
                                     const xfce_session_display *display)
{
    (void) environment;
    session_export(display);
}

static void refresh_settings(xfce_environment *environment)
{
    XfconfChannel *xfwm4 = channels_get(environment->channels, CHANNEL_XFWM4);

    settings_store_missing_defaults(xfwm4);
    g_free(environment->theme_name);
    g_free(environment->title_font);
    environment->theme_name = settings_read_text(xfwm4, "theme");
    environment->title_font = settings_read_text(xfwm4, "title_font");
}

static void reload_keymap(xfce_environment *environment)
{
    keymap_store *previous = environment->keymap;

    environment->keymap = shortcuts_load(channels_get(environment->channels, CHANNEL_SHORTCUTS));
    if (previous) {
        keymap_store_destroy(previous);
    }
}

static void on_channel_changed(void *self, channel_id id, const char *property)
{
    xfce_environment *environment = self;

    dispatcher_post(environment->dispatcher, change_flags_for(id, property));
}

static void on_dispatch(void *self, unsigned flags)
{
    xfce_environment *environment = self;

    if (flags & (CHANGE_THEME | CHANGE_SETTINGS)) {
        refresh_settings(environment);
        notify_settings(environment, flags);
    }
    if (flags & CHANGE_SHORTCUTS) {
        reload_keymap(environment);
        notify(environment, environment->listener.shortcuts_changed);
    }
    if (flags & CHANGE_INPUT) {
        notify(environment, environment->listener.input_changed);
    }
}

static void notify_settings(xfce_environment *environment, unsigned flags)
{
    if (flags & CHANGE_THEME) {
        notify(environment, environment->listener.theme_changed);
    } else {
        notify(environment, environment->listener.settings_changed);
    }
}

static void on_style_changed(void *self)
{
    xfce_environment *environment = self;

    notify(environment, environment->listener.style_changed);
}

static void notify(xfce_environment *environment, void (*callback)(void *self))
{
    if (callback) {
        callback(environment->listener.self);
    }
}

static keyboard_prefs keyboard_prefs_of(void *self)
{
    xfce_environment *environment = self;

    return input_prefs_keyboard(environment->channels);
}

static pointer_prefs pointer_prefs_of(void *self, const char *device_name)
{
    xfce_environment *environment = self;

    return input_prefs_pointer(environment->channels, device_name);
}

static cursor_prefs cursor_prefs_of(void *self)
{
    xfce_environment *environment = self;

    return input_prefs_cursor(environment->channels);
}

static void lookup_icon(void *self, const icon_request *request)
{
    xfce_environment *environment = self;

    gtk_bridge_lookup_icon(environment->bridge, request);
}

static void cancel_icon_lookups(void *self, void *context)
{
    xfce_environment *environment = self;

    gtk_bridge_cancel_icons(environment->bridge, context);
}

static void ask_terminate(void *self, const terminate_request *request)
{
    xfce_environment *environment = self;

    gtk_bridge_ask_terminate(environment->bridge, request);
}
