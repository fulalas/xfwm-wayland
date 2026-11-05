#ifndef COMPOSITOR_X11_SETTINGS_H
#define COMPOSITOR_X11_SETTINGS_H

#include <stdint.h>

enum {
    X11_SETTING_XFT_DPI_UNIT = 1024,
};

typedef enum x11_setting_type {
    X11_SETTING_INTEGER,
    X11_SETTING_STRING,
} x11_setting_type;

typedef struct x11_setting {
    const char *name;
    x11_setting_type type;
    int32_t integer;
    const char *string;
} x11_setting;

typedef struct x11_setting_sink {
    void *self;
    void (*add)(void *self, const x11_setting *setting);
} x11_setting_sink;

#endif
