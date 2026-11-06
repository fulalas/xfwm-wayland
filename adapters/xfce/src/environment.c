#include <stdlib.h>
#include "xfce/environment.h"

#include <glib.h>

#include "accessibility.h"
#include "channels.h"
#include "dispatcher.h"
#include "display_off.h"
#include "display_store.h"
#include "gtk_bridge.h"
#include "input_prefs.h"
#include "logout.h"
#include "power_source.h"
#include "screensaver_service.h"
#include "session_export.h"
#include "session_launch.h"
#include "settings.h"
#include "shortcuts.h"
#include "shortcuts_portal.h"
#include "spawn.h"
#include "x11_settings.h"
#include "xfconf_source.h"

struct xfce_environment {
    xfce_listener listener;
    channels *channels;
    dispatcher *dispatcher;
    keymap_store *keymap;
    shortcuts_portal *portal;
    power_source *power;
    screensaver_service *screensaver;
    gtk_bridge *bridge;
    session_launch *session;
    char *theme_name;
    char *title_font;
};

static void refresh_settings(xfce_environment *environment);
static void reload_keymap(xfce_environment *environment);
static void on_channel_changed(void *self, channel_id id, const char *property);
static void on_dispatch(void *self, unsigned flags);
static void notify_settings(xfce_environment *environment, unsigned flags);
static void on_style_changed(void *self);
static void on_session_ended(void *self);
static void on_app_shortcuts_changed(void *self);
static void on_power_source_changed(void *self);
static uint32_t display_off_ms_of(void *self);
static bool idle_inhibited_of(void *self);
static void on_screensaver_activity(void *self);
static void notify(xfce_environment *environment, void (*callback)(void *self));
static keyboard_prefs keyboard_prefs_of(void *self);
static pointer_prefs pointer_prefs_of(void *self, const char *device_name);
static cursor_prefs cursor_prefs_of(void *self);
static accessibility_prefs accessibility_prefs_of(void *self);
static void lookup_icon(void *self, const icon_request *request);
static void cancel_icon_lookups(void *self, void *context);
static void ask_terminate(void *self, const terminate_request *request);
static void x11_settings_of(void *self, const x11_setting_sink *sink);
static display_plan load_displays(void *self, const display_set *connected);
static void save_displays(void *self, const display_set *current);

xfce_environment *xfce_environment_create(const xfce_listener *listener)
{
    xfce_environment *environment = g_new0(xfce_environment, 1);
    channel_watch watch = { .self = environment, .changed = on_channel_changed };
    dispatch_target target = { .self = environment, .run = on_dispatch };
    bridge_listener bridge_listener = { .self = environment, .style_changed = on_style_changed };
    shortcuts_portal_listener portal_listener = { environment, on_app_shortcuts_changed };
    power_source_listener power_listener = { environment, on_power_source_changed };
    screensaver_listener screensaver_listener = { environment, on_power_source_changed,
                                                  on_screensaver_activity };

    environment->listener = *listener;
    environment->channels = channels_create(&watch);
    environment->dispatcher = dispatcher_create(&target);
    environment->bridge = gtk_bridge_create(&bridge_listener);
    environment->portal = shortcuts_portal_create(&portal_listener);
    environment->power = power_source_create(&power_listener);
    environment->screensaver = screensaver_service_create(&screensaver_listener);
    settings_store_missing_defaults(channels_get(environment->channels, CHANNEL_XFWM4));
    refresh_settings(environment);
    reload_keymap(environment);
    return environment;
}

void xfce_environment_destroy(xfce_environment *environment)
{
    session_launch_destroy(environment->session);
    shortcuts_portal_destroy(environment->portal);
    power_source_destroy(environment->power);
    screensaver_service_destroy(environment->screensaver);
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
    wm_keymap keymap = keymap_store_view(environment->keymap);

    keymap.app_shortcut_count = shortcuts_portal_keymap(environment->portal,
                                                        &keymap.app_shortcuts);
    return keymap;
}

void xfce_environment_app_shortcut(xfce_environment *environment, uint32_t id, bool pressed,
                                   uint32_t time_ms)
{
    shortcuts_portal_activate(environment->portal, id, pressed, time_ms);
}

xfce_style xfce_environment_style(xfce_environment *environment)
{
    return gtk_bridge_style(environment->bridge);
}

int xfce_environment_font_dpi(xfce_environment *environment)
{
    XfconfChannel *xsettings = channels_get(environment->channels, CHANNEL_XSETTINGS);
    int dpi = xfconf_channel_get_int(xsettings, "/Xft/DPI", 0);

    return dpi > 0 ? dpi : 0;
}

compositor_services xfce_environment_services(xfce_environment *environment)
{
    return (compositor_services) {
        .self = environment,
        .keyboard_prefs = keyboard_prefs_of,
        .pointer_prefs = pointer_prefs_of,
        .cursor_prefs = cursor_prefs_of,
        .accessibility_prefs = accessibility_prefs_of,
        .lookup_icon = lookup_icon,
        .cancel_icon_lookups = cancel_icon_lookups,
        .ask_terminate = ask_terminate,
        .load_displays = load_displays,
        .save_displays = save_displays,
        .x11_settings = x11_settings_of,
        .display_off_ms = display_off_ms_of,
        .idle_inhibited = idle_inhibited_of,
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
    (void) environment;
    logout_request();
}

void xfce_environment_export_session(xfce_environment *environment,
                                     const xfce_session_display *display)
{
    (void) environment;
    session_export(display);
}

void xfce_environment_display_changed(xfce_environment *environment,
                                      const xfce_session_display *display)
{
    session_export(display);
    if (environment->session) {
        session_export_update_activation();
    }
}

void xfce_environment_start_session(xfce_environment *environment,
                                    const xfce_session_display *display, const char *command)
{
    session_launch_listener listener = { .self = environment, .ended = on_session_ended };

    session_export(display);
    session_launch_destroy(environment->session);
    environment->session = session_launch_start(command, &listener);
}

static void refresh_settings(xfce_environment *environment)
{
    XfconfChannel *xfwm4 = channels_get(environment->channels, CHANNEL_XFWM4);

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
    if (flags & CHANGE_X11_SETTINGS) {
        notify(environment, environment->listener.x11_settings_changed);
    }
    if (flags & CHANGE_DISPLAYS) {
        notify(environment, environment->listener.displays_changed);
    }
    if (flags & CHANGE_POWER) {
        notify(environment, environment->listener.power_changed);
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

static void on_session_ended(void *self)
{
    xfce_environment *environment = self;

    notify(environment, environment->listener.session_ended);
}

static void on_power_source_changed(void *self)
{
    xfce_environment *environment = self;

    dispatcher_post(environment->dispatcher, CHANGE_POWER);
}

static uint32_t display_off_ms_of(void *self)
{
    xfce_environment *environment = self;

    if (power_source_manager_running(environment->power)) {
        return 0;
    }
    xfconf_source *source = xfconf_source_create(
        channels_get(environment->channels, CHANNEL_POWER_MANAGER), "/xfce4-power-manager/");
    setting_source view = xfconf_source_view(source);
    uint32_t ms = display_off_ms_from(&view, power_source_on_battery(environment->power));

    xfconf_source_destroy(source);
    return ms;
}

static bool idle_inhibited_of(void *self)
{
    xfce_environment *environment = self;

    return screensaver_service_inhibited(environment->screensaver);
}

static void on_screensaver_activity(void *self)
{
    xfce_environment *environment = self;

    notify(environment, environment->listener.user_activity);
}

static void on_app_shortcuts_changed(void *self)
{
    xfce_environment *environment = self;

    notify(environment, environment->listener.shortcuts_changed);
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

static accessibility_prefs accessibility_prefs_of(void *self)
{
    xfce_environment *environment = self;

    return accessibility_prefs_load(environment->channels);
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

static display_plan load_displays(void *self, const display_set *connected)
{
    xfce_environment *environment = self;

    return display_store_load(channels_get(environment->channels, CHANNEL_DISPLAYS), connected);
}

static void save_displays(void *self, const display_set *current)
{
    xfce_environment *environment = self;

    display_store_save(channels_get(environment->channels, CHANNEL_DISPLAYS), current);
}

static void x11_settings_of(void *self, const x11_setting_sink *sink)
{
    xfce_environment *environment = self;

    x11_settings_collect(channels_get(environment->channels, CHANNEL_XSETTINGS), sink);
}

int xfce_environment_session_exit_code(const xfce_environment *environment)
{
    return environment->session ? session_launch_exit_code(environment->session) : EXIT_SUCCESS;
}
