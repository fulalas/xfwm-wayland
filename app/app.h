#ifndef APP_APP_H
#define APP_APP_H

#include <glib.h>
#include <stdbool.h>

#include "appearance/appearance.h"
#include "compositor/compositor.h"
#include "wm/desktop.h"
#include "xfce/environment.h"

typedef struct app_options {
    bool show_version;
    const char *startup_command;
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
void app_export_session_environment(app *app);
void app_run_startup_command(app *app);

#endif
