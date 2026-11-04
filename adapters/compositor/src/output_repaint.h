#ifndef COMPOSITOR_OUTPUT_REPAINT_H
#define COMPOSITOR_OUTPUT_REPAINT_H

#include <stdbool.h>

#include "output.h"

struct wlr_renderer;
struct wlr_scene_timer;

bool output_repaint_renderer_can_time(struct wlr_renderer *renderer);
bool output_repaint_init(output *output);
void output_repaint_finish(output *output);
void output_repaint_frame(output *output);
struct wlr_scene_timer *output_repaint_timer(output *output);

#endif
