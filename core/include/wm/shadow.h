#ifndef WM_SHADOW_H
#define WM_SHADOW_H

#include "wm/config.h"
#include "wm/geometry.h"

enum {
    WM_SHADOW_RADIUS = 12,
};

int wm_shadow_kernel_size(void);
wm_rect wm_shadow_rect(const wm_rect *frame, const wm_compositing_config *config);

#endif
