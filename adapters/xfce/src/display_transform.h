#ifndef XFCE_DISPLAY_TRANSFORM_H
#define XFCE_DISPLAY_TRANSFORM_H

#include "compositor/displays.h"

typedef struct display_rotation {
    int degrees;
    const char *reflection;
} display_rotation;

display_transform display_transform_from(const display_rotation *rotation);
display_rotation display_rotation_from(display_transform transform);

#endif
