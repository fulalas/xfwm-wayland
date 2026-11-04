#ifndef COMPOSITOR_OUTPUT_COLOR_H
#define COMPOSITOR_OUTPUT_COLOR_H

#include <stdint.h>

#include "output.h"

struct wlr_output_state;

uint32_t output_color_wanted_format(const compositor *server);
uint32_t output_color_fallback_format(void);
void output_color_describe(output *output, struct wlr_output_state *state);
void outputs_apply_color_depth(outputs *outputs);

#endif
