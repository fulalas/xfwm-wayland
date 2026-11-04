#ifndef COMPOSITOR_OUTPUT_COLOR_H
#define COMPOSITOR_OUTPUT_COLOR_H

#include "output.h"

struct wlr_output_state;

void output_color_describe(output *output, struct wlr_output_state *state);
void outputs_apply_color_depth(outputs *outputs);

#endif
