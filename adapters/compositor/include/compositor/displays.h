#ifndef COMPOSITOR_DISPLAYS_H
#define COMPOSITOR_DISPLAYS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DISPLAY_MAX = 16,
};

typedef enum display_transform {
    DISPLAY_TRANSFORM_NORMAL,
    DISPLAY_TRANSFORM_90,
    DISPLAY_TRANSFORM_180,
    DISPLAY_TRANSFORM_270,
    DISPLAY_TRANSFORM_FLIPPED,
    DISPLAY_TRANSFORM_FLIPPED_90,
    DISPLAY_TRANSFORM_FLIPPED_180,
    DISPLAY_TRANSFORM_FLIPPED_270,
} display_transform;

typedef struct display_identity {
    const char *connector;
    const char *description;
    const char *make;
    const char *model;
    const char *serial;
    const uint8_t *edid;
    size_t edid_size;
} display_identity;

typedef struct display_mode {
    int width;
    int height;
    int refresh_mhz;
} display_mode;

typedef struct display_setting {
    bool enabled;
    display_mode mode;
    int x;
    int y;
    display_transform transform;
    double scale;
} display_setting;

typedef struct display_state {
    display_identity identity;
    display_setting setting;
    bool is_primary;
} display_state;

typedef struct display_set {
    const display_state *displays;
    size_t count;
} display_set;

typedef struct display_saved {
    bool is_known;
    bool is_primary;
    display_setting setting;
} display_saved;

typedef struct display_plan {
    display_saved displays[DISPLAY_MAX];
} display_plan;

#endif
