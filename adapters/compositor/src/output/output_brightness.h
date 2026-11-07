#ifndef COMPOSITOR_OUTPUT_BRIGHTNESS_H
#define COMPOSITOR_OUTPUT_BRIGHTNESS_H

#include "output.h"

struct wlr_output_state;

void output_brightness_init(output *output);
void output_brightness_frame(output *output, struct wlr_output_state *state);
void output_brightness_committed(output *output, const struct wlr_output_state *state);
void output_brightness_release(output *output);
void outputs_brightness_changed(outputs *outputs);

#endif
