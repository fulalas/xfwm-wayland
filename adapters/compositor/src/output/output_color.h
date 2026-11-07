#ifndef COMPOSITOR_OUTPUT_COLOR_H
#define COMPOSITOR_OUTPUT_COLOR_H

#include <stdbool.h>

#include "output.h"

struct wlr_output_state;

void output_color_describe(output *output, struct wlr_output_state *state);
void output_color_describe_basic(output *output, struct wlr_output_state *state);
bool outputs_apply_color(outputs *outputs);

#endif
