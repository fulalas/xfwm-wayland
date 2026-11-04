#include "wm/shadow.h"

#include <math.h>

enum {
    KERNEL_RADIUS_SPAN = 3,
    SHADOW_OFFSET = -3 * WM_SHADOW_RADIUS / 2,
};

int wm_shadow_kernel_size(void)
{
    return ((int) ceil((double) WM_SHADOW_RADIUS * KERNEL_RADIUS_SPAN) + 1) & ~1;
}

wm_rect wm_shadow_rect(const wm_rect *frame, const wm_compositing_config *config)
{
    int kernel = wm_shadow_kernel_size();
    return (wm_rect) {
        frame->x + SHADOW_OFFSET - config->shadow_delta_x,
        frame->y + SHADOW_OFFSET - config->shadow_delta_y,
        frame->width + kernel + config->shadow_delta_width + config->shadow_delta_x,
        frame->height + kernel + config->shadow_delta_height + config->shadow_delta_y,
    };
}
