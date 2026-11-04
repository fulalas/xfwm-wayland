#ifndef COMPOSITOR_SCALABLE_CURSOR_H
#define COMPOSITOR_SCALABLE_CURSOR_H

#include <stddef.h>
#include <stdint.h>

struct wlr_buffer;

typedef struct scalable_theme scalable_theme;

typedef struct cursor_frame {
    struct wlr_buffer *buffer;
    int32_t hotspot_x;
    int32_t hotspot_y;
    float scale;
    uint32_t delay_ms;
} cursor_frame;

typedef struct cursor_animation cursor_animation;

typedef struct scalable_theme_request {
    const char *name;
    int size;
    double scale;
} scalable_theme_request;

scalable_theme *scalable_theme_open(const scalable_theme_request *request);
void scalable_theme_close(scalable_theme *theme);
cursor_animation *scalable_theme_cursor(scalable_theme *theme, const char *name);
size_t cursor_animation_length(const cursor_animation *animation);
const cursor_frame *cursor_animation_frame(cursor_animation *animation, size_t index);

#endif
