#ifndef COMPOSITOR_USABLE_AREA_H
#define COMPOSITOR_USABLE_AREA_H

#include "wm/geometry.h"

typedef struct strut_range {
    int size;
    int start;
    int end;
} strut_range;

typedef struct strut {
    strut_range left;
    strut_range right;
    strut_range top;
    strut_range bottom;
} strut;

typedef struct strut_context {
    wm_rect output;
    wm_rect root;
} strut_context;

wm_rect usable_area_apply_strut(wm_rect usable, const strut_context *context, const strut *s);

#endif
