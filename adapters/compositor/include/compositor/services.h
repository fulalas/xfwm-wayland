#ifndef COMPOSITOR_SERVICES_H
#define COMPOSITOR_SERVICES_H

#include <stdbool.h>
#include <cairo.h>

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

typedef struct pointer_prefs {
    optional_bool tap;
    optional_bool natural_scroll;
    optional_bool left_handed;
    optional_double accel_speed;
    optional_bool disable_while_typing;
    optional_bool middle_emulation;
} pointer_prefs;

typedef struct cursor_prefs {
    char theme[PREFS_TEXT_MAX];
    int size;
} cursor_prefs;

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
    void (*lookup_icon)(void *self, const icon_request *request);
    void (*cancel_icon_lookups)(void *self, void *context);
    void (*ask_terminate)(void *self, const terminate_request *request);
} compositor_services;

#endif
