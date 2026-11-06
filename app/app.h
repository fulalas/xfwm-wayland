#ifndef APP_APP_H
#define APP_APP_H

#include <glib.h>
#include <stdbool.h>

#include "appearance/appearance.h"
#include "compositor/compositor.h"
#include "wm/desktop.h"
#include "xfce/environment.h"

typedef struct config_overrides {
    bool has_compositing;
    bool compositing;
    bool has_vblank;
    wm_vblank_mode vblank;
    bool has_setting;
    bool setting_compositing;
    wm_vblank_mode setting_vblank;
} config_overrides;

typedef struct app_options {
    bool show_version;
    bool no_session;
    const char *startup_command;
    config_overrides overrides;
} app_options;

typedef struct app {
    GMainLoop *loop;
    GSource *wayland_source;
    xfce_environment *xfce;
    appearance *appearance;
    compositor *compositor;
    wm_desktop *desktop;
    compositor_painter painter;
    compositor_services services;
    compositor_listener compositor_listener;
    wm_config config;
    config_overrides overrides;
    double max_output_scale;
    char *startup_command;
} app;

app *app_create(const app_options *options);
void app_destroy(app *app);
int app_run(app *app);

void app_load_appearance(app *app);
void app_push_appearance(app *app);
void app_reload_appearance(app *app);
void app_reload_theme(app *app);
void app_reload_config(app *app);
void app_reload_keymap(app *app);
wm_commands app_commands(app *app);
GSource *app_wayland_source_new(compositor *compositor);
void app_start_session(app *app);
void app_export_x11_display(app *app);
void app_realtime_start(void);
GPid app_session_bus_start(void);
void app_session_bus_stop(GPid bus);

#endif
