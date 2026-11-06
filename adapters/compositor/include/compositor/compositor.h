#ifndef COMPOSITOR_COMPOSITOR_H
#define COMPOSITOR_COMPOSITOR_H

#include <stdbool.h>

#include "compositor/painter.h"
#include "compositor/services.h"
#include "wm/config.h"
#include "wm/desktop.h"
#include "wm/ports.h"

typedef struct compositor compositor;

typedef struct compositor_listener {
    void *self;
    void (*started)(void *self);
    void (*output_scale_changed)(void *self, double max_scale);
    void (*quit_requested)(void *self);
    void (*x11_display_changed)(void *self);
} compositor_listener;

typedef struct compositor_options {
    const compositor_painter *painter;
    const compositor_services *services;
    const compositor_listener *listener;
    const wm_config *config;
} compositor_options;

compositor *compositor_create(const compositor_options *options);
void compositor_destroy(compositor *compositor);
bool compositor_start(compositor *compositor, wm_desktop *desktop);

wm_presenter compositor_presenter(compositor *compositor);
wm_environment compositor_environment(compositor *compositor);
void compositor_configure_window(compositor *compositor, void *window, const wm_rect *client);
void compositor_close_window(compositor *compositor, void *window);
void compositor_ask_terminate(compositor *compositor, void *window);
void compositor_warp_pointer(compositor *compositor, wm_point point);
void compositor_schedule_timer(compositor *compositor, uint32_t delay_ms);

void compositor_set_config(compositor *compositor, const wm_config *config);
void compositor_appearance_changed(compositor *compositor);
void compositor_input_prefs_changed(compositor *compositor);
void compositor_display_prefs_changed(compositor *compositor);
void compositor_power_prefs_changed(compositor *compositor);
void compositor_user_activity(compositor *compositor);
void compositor_x11_settings_changed(compositor *compositor);

int compositor_event_fd(compositor *compositor);
void compositor_dispatch(compositor *compositor);
void compositor_flush(compositor *compositor);
const char *compositor_socket_name(compositor *compositor);
const char *compositor_x11_display(compositor *compositor);

#endif
