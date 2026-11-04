#ifndef COMPOSITOR_ABSOLUTE_INPUT_H
#define COMPOSITOR_ABSOLUTE_INPUT_H

#include "compositor/services.h"
#include "input_transform.h"

struct wlr_cursor;
struct wlr_input_device;
struct wlr_output_layout;

typedef struct absolute_input absolute_input;

typedef struct absolute_places {
    struct wlr_cursor *cursor;
    struct wlr_output_layout *layout;
} absolute_places;

absolute_input *absolute_input_create(const absolute_places *places);
void absolute_input_destroy(absolute_input *absolute);
void absolute_input_configure(absolute_input *absolute, struct wlr_input_device *device,
                              const absolute_prefs *prefs);
input_point absolute_input_position(const absolute_input *absolute,
                                    struct wlr_input_device *device, input_point where);
input_point absolute_input_tilt(const absolute_input *absolute, struct wlr_input_device *device,
                                input_point tilt);

#endif
