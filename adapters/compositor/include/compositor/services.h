#ifndef COMPOSITOR_SERVICES_H
#define COMPOSITOR_SERVICES_H

#include <stdbool.h>
#include <cairo.h>

#include "compositor/displays.h"
#include "compositor/x11_settings.h"

enum {
    PREFS_TEXT_MAX = 256,
};

typedef struct keyboard_prefs {
    char layout[PREFS_TEXT_MAX];
    char variant[PREFS_TEXT_MAX];
    char model[PREFS_TEXT_MAX];
    char options[PREFS_TEXT_MAX];
    int repeat_rate;
    int repeat_delay_ms;
    bool numlock;
} keyboard_prefs;

typedef struct optional_bool {
    bool is_set;
    bool value;
} optional_bool;

typedef struct optional_double {
    bool is_set;
    double value;
} optional_double;

typedef struct optional_int {
    bool is_set;
    int value;
} optional_int;

typedef enum pointer_accel_profile {
    POINTER_ACCEL_PROFILE_DEFAULT,
    POINTER_ACCEL_PROFILE_ADAPTIVE,
    POINTER_ACCEL_PROFILE_FLAT,
} pointer_accel_profile;

typedef enum pointer_click_method {
    POINTER_CLICK_METHOD_DEFAULT,
    POINTER_CLICK_METHOD_NONE,
    POINTER_CLICK_METHOD_BUTTON_AREAS,
    POINTER_CLICK_METHOD_CLICKFINGER,
} pointer_click_method;

typedef enum pointer_scroll_method {
    POINTER_SCROLL_METHOD_DEFAULT,
    POINTER_SCROLL_METHOD_NONE,
    POINTER_SCROLL_METHOD_TWO_FINGER,
    POINTER_SCROLL_METHOD_EDGE,
    POINTER_SCROLL_METHOD_ON_BUTTON_DOWN,
} pointer_scroll_method;

enum {
    INPUT_REFLECT_X = 1,
    INPUT_REFLECT_Y = 2,
    CALIBRATION_VALUES = 6,
    OUTPUT_ID_MAX = 64,
};

typedef struct calibration_prefs {
    bool is_set;
    float matrix[CALIBRATION_VALUES];
} calibration_prefs;

typedef struct absolute_prefs {
    char output_id[OUTPUT_ID_MAX];
    int rotation_degrees;
    unsigned reflection;
    calibration_prefs calibration;
} absolute_prefs;

typedef struct pointer_prefs {
    optional_bool tap;
    optional_bool natural_scroll;
    optional_bool left_handed;
    optional_double accel_speed;
    optional_bool disable_while_typing;
    optional_bool middle_emulation;
    optional_bool enabled;
    pointer_accel_profile accel_profile;
    pointer_click_method click_method;
    pointer_scroll_method scroll_method;
    optional_int scroll_button;
    optional_bool scroll_button_lock;
    absolute_prefs absolute;
} pointer_prefs;

typedef struct cursor_prefs {
    char theme[PREFS_TEXT_MAX];
    int size;
} cursor_prefs;

typedef struct accessibility_prefs {
    bool find_cursor;
} accessibility_prefs;

typedef struct icon_request {
    const char *app_id;
    const char *icon_name;
    int size;
    void (*done)(void *context, cairo_surface_t *icon);
    void *context;
} icon_request;

typedef struct terminate_request {
    const char *title;
    void (*done)(void *context, bool terminate);
    void *context;
} terminate_request;

typedef struct compositor_services {
    void *self;
    keyboard_prefs (*keyboard_prefs)(void *self);
    pointer_prefs (*pointer_prefs)(void *self, const char *device_name);
    cursor_prefs (*cursor_prefs)(void *self);
    accessibility_prefs (*accessibility_prefs)(void *self);
    void (*lookup_icon)(void *self, const icon_request *request);
    void (*cancel_icon_lookups)(void *self, void *context);
    void (*ask_terminate)(void *self, const terminate_request *request);
    display_plan (*load_displays)(void *self, const display_set *connected);
    void (*save_displays)(void *self, const display_set *current);
    void (*x11_settings)(void *self, const x11_setting_sink *sink);
} compositor_services;

#endif
