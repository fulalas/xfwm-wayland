#ifndef COMPOSITOR_SCALE_GUESS_H
#define COMPOSITOR_SCALE_GUESS_H

#include "wm/geometry.h"

typedef struct scale_guess_panel {
    wm_size pixels;
    wm_size millimeters;
} scale_guess_panel;

double scale_guess(const scale_guess_panel *panel);

#endif
