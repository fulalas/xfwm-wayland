#ifndef APPEARANCE_COLOR_H
#define APPEARANCE_COLOR_H

#include <stdbool.h>
#include <stdint.h>

#include "appearance/appearance.h"

typedef struct xpm_rgb {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} xpm_rgb;

bool appearance_xpm_color_parse(const char *spec, xpm_rgb *rgb);
bool appearance_color_parse(const char *spec, ui_color *color);

#endif
