#include <glib-unix.h>
#include <signal.h>
#include <stdlib.h>

#include "app.h"

static bool create_adapters(app *app);
static bool create_compositor(app *app);
static bool create_desktop(app *app);
static void connect_signals(app *app);
static void on_compositor_started(void *self);
static void on_output_scale_changed(void *self, double max_scale);
static void on_quit_requested(void *self);
static void on_theme_changed(void *self);
static void on_settings_changed(void *self);
static void on_shortcuts_changed(void *self);
static void on_input_changed(void *self);
static void on_displays_changed(void *self);
static void on_x11_settings_changed(void *self);
static void on_x11_display_changed(void *self);
static void on_style_changed(void *self);
static gboolean on_terminate_signal(gpointer data);

app *app_create(const app_options *options)
{
    app *app = calloc(1, sizeof(*app));

    app->startup_command = g_strdup(options->startup_command);
    app->max_output_scale = 1.0;
    app->loop = g_main_loop_new(NULL, FALSE);
    if (!create_adapters(app) || !create_compositor(app) || !create_desktop(app)) {
        app_destroy(app);
        return NULL;
    }
    connect_signals(app);
    return app;
}

void app_destroy(app *app)
{
    if (app->wayland_source) {
        g_source_destroy(app->wayland_source);
        g_source_unref(app->wayland_source);
    }
    if (app->compositor) {
        compositor_destroy(app->compositor);
    }
    wm_desktop_destroy(app->desktop);
    if (app->appearance) {
        appearance_destroy(app->appearance);
    }
    if (app->xfce) {
        xfce_environment_destroy(app->xfce);
    }
    g_main_loop_unref(app->loop);
    g_free(app->startup_command);
    free(app);
}

int app_run(app *app)
{
    if (!compositor_start(app->compositor, app->desktop)) {
        g_warning("Cannot start the Wayland compositor");
        return EXIT_FAILURE;
    }
    app->wayland_source = app_wayland_source_new(app->compositor);
    g_source_attach(app->wayland_source, NULL);
    g_main_loop_run(app->loop);
    return xfce_environment_session_exit_code(app->xfce);
}

static bool create_adapters(app *app)
{
    xfce_listener listener = {
        .self = app,
        .theme_changed = on_theme_changed,
        .settings_changed = on_settings_changed,
        .shortcuts_changed = on_shortcuts_changed,
        .input_changed = on_input_changed,
        .displays_changed = on_displays_changed,
        .x11_settings_changed = on_x11_settings_changed,
        .style_changed = on_style_changed,
        .session_ended = on_quit_requested,
    };

    app->xfce = xfce_environment_create(&listener);
    app->appearance = appearance_create();
    if (!app->xfce || !app->appearance) {
        return false;
    }
    app->painter = appearance_painter(app->appearance);
    app->services = xfce_environment_services(app->xfce);
    app_load_appearance(app);
    return true;
}

static bool create_compositor(app *app)
{
    compositor_options options;

    app->compositor_listener = (compositor_listener) {
        .self = app,
        .started = on_compositor_started,
        .output_scale_changed = on_output_scale_changed,
        .quit_requested = on_quit_requested,
        .x11_display_changed = on_x11_display_changed,
    };
    options = (compositor_options) {
        .painter = &app->painter,
        .services = &app->services,
        .listener = &app->compositor_listener,
        .config = &app->config,
    };
    app->compositor = compositor_create(&options);
    return app->compositor != NULL;
}

static bool create_desktop(app *app)
{
    wm_ports ports = {
        .presenter = compositor_presenter(app->compositor),
        .commands = app_commands(app),
        .environment = compositor_environment(app->compositor),
    };

    app->desktop = wm_desktop_create(&ports, &app->config);
    if (!app->desktop) {
        return false;
    }
    app_push_appearance(app);
    app_reload_keymap(app);
    return true;
}

static void connect_signals(app *app)
{
    g_unix_signal_add(SIGINT, on_terminate_signal, app);
    g_unix_signal_add(SIGTERM, on_terminate_signal, app);
    signal(SIGPIPE, SIG_IGN);
}

static void on_compositor_started(void *self)
{
    app *app = self;

    app_start_session(app);
    xfce_environment_start_helper(app->xfce);
}

static void on_output_scale_changed(void *self, double max_scale)
{
    app *app = self;

    app->max_output_scale = max_scale;
    app_reload_appearance(app);
}

static void on_quit_requested(void *self)
{
    app *app = self;

    g_main_loop_quit(app->loop);
}

static void on_theme_changed(void *self)
{
    app_reload_theme(self);
}

static void on_settings_changed(void *self)
{
    app_reload_config(self);
}

static void on_shortcuts_changed(void *self)
{
    app_reload_keymap(self);
}

static void on_input_changed(void *self)
{
    app *app = self;

    compositor_input_prefs_changed(app->compositor);
}

static void on_displays_changed(void *self)
{
    app *app = self;

    compositor_display_prefs_changed(app->compositor);
}

static void on_x11_settings_changed(void *self)
{
    app *app = self;

    compositor_x11_settings_changed(app->compositor);
    app_reload_appearance(app);
}

static void on_x11_display_changed(void *self)
{
    app_export_x11_display(self);
}

static void on_style_changed(void *self)
{
    app_reload_appearance(self);
}

static gboolean on_terminate_signal(gpointer data)
{
    app *app = data;

    g_main_loop_quit(app->loop);
    return G_SOURCE_CONTINUE;
}
