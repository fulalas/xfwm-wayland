#include "frame_bands.h"

frame_bands frame_bands_compute(const wm_decor_layout *layout)
{
    const wm_extents *e = &layout->extents;
    int width = layout->frame_size.width;
    int height = layout->frame_size.height;
    int middle = wm_max(height - e->top - e->bottom, 0);
    frame_bands bands = {
        .rects = {
            [FRAME_BAND_TOP] = {0, 0, width, e->top},
            [FRAME_BAND_BOTTOM] = {0, height - e->bottom, width, e->bottom},
            [FRAME_BAND_LEFT] = {0, e->top, e->left, middle},
            [FRAME_BAND_RIGHT] = {width - e->right, e->top, e->right, middle},
        },
    };
    return bands;
}
