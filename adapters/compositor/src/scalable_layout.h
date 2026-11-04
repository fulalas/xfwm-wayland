#ifndef COMPOSITOR_SCALABLE_LAYOUT_H
#define COMPOSITOR_SCALABLE_LAYOUT_H

typedef struct scalable_request {
    double canvas_size;
    double nominal_size;
    int cursor_size;
    double output_scale;
} scalable_request;

typedef struct scalable_layout {
    int pixels;
    float buffer_scale;
    double hotspot_factor;
} scalable_layout;

scalable_layout scalable_layout_for(const scalable_request *request);

#endif
