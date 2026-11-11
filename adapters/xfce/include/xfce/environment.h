#ifndef XFCE_ENVIRONMENT_H
#define XFCE_ENVIRONMENT_H

#include <stdbool.h>
#include <stddef.h>

#include "compositor/services.h"
#include "wm/config.h"
#include "wm/keys.h"
#include "wm/ports.h"
#include "wm/rules.h"

enum {
    XFCE_SYMBOL_MAX = 32,
    XFCE_SYMBOL_NAME_MAX = 48,
    XFCE_SYMBOL_VALUE_MAX = 32,
    XFCE_FONT_MAX = 128,
};

typedef struct xfce_color {
    double red;
    double green;
    double blue;
    double alpha;
} xfce_color;

typedef struct xfce_color_symbol {
    char name[XFCE_SYMBOL_NAME_MAX];
    char value[XFCE_SYMBOL_VALUE_MAX];
} xfce_color_symbol;

typedef struct xfce_style {
    xfce_color_symbol symbols[XFCE_SYMBOL_MAX];
    size_t symbol_count;
    xfce_color background;
    xfce_color foreground;
    xfce_color selected_background;
    xfce_color selected_foreground;
    xfce_color border;
    xfce_color insensitive_foreground;
    char font[XFCE_FONT_MAX];
    bool is_from_gtk;
} xfce_style;

typedef struct xfce_setting_source {
    void *self;
    const char *(*lookup)(void *self, const char *key);
} xfce_setting_source;

typedef struct xfce_window_rules {
    wm_window_rule *items;
    size_t count;
} xfce_window_rules;

typedef struct xfce_session_display {
    const char *wayland_display;
    const char *x11_display;
} xfce_session_display;

typedef struct xfce_listener {
    void *self;
    void (*theme_changed)(void *self);
    void (*settings_changed)(void *self);
    void (*window_rules_changed)(void *self);
    void (*shortcuts_changed)(void *self);
    void (*input_changed)(void *self);
    void (*style_changed)(void *self);
    void (*session_ended)(void *self);
    void (*displays_changed)(void *self);
    void (*x11_settings_changed)(void *self);
    void (*power_changed)(void *self);
    void (*user_activity)(void *self);
} xfce_listener;

typedef struct xfce_environment xfce_environment;

xfce_environment *xfce_environment_create(const xfce_listener *listener);
void xfce_environment_destroy(xfce_environment *environment);
void xfce_environment_start_helper(xfce_environment *environment);

const char *xfce_environment_theme_name(xfce_environment *environment);
const char *xfce_environment_title_font(xfce_environment *environment);
wm_config xfce_environment_config(xfce_environment *environment,
                                  const xfce_setting_source *theme);
wm_keymap xfce_environment_keymap(xfce_environment *environment);
xfce_window_rules xfce_environment_window_rules(xfce_environment *environment);
void xfce_window_rules_free(xfce_window_rules *rules);
void xfce_environment_app_shortcut(xfce_environment *environment,
                                  const wm_app_shortcut_event *event);
xfce_style xfce_environment_style(xfce_environment *environment);
compositor_services xfce_environment_services(xfce_environment *environment);

void xfce_environment_store_workspace_count(xfce_environment *environment, int count);
void xfce_environment_store_workspace_rows(xfce_environment *environment, int rows);
void xfce_environment_store_workspace_names(xfce_environment *environment,
                                            const char *const *names, size_t count);
void xfce_environment_spawn(xfce_environment *environment, const char *command);
void xfce_environment_logout(xfce_environment *environment);
int xfce_environment_session_exit_code(const xfce_environment *environment);
void xfce_environment_export_session(xfce_environment *environment,
                                     const xfce_session_display *display);
void xfce_environment_display_changed(xfce_environment *environment,
                                      const xfce_session_display *display);
void xfce_environment_start_session(xfce_environment *environment,
                                    const xfce_session_display *display, const char *command);

#endif
