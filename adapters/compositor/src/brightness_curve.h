#ifndef COMPOSITOR_BRIGHTNESS_CURVE_H
#define COMPOSITOR_BRIGHTNESS_CURVE_H

#include <stddef.h>
#include <stdint.h>

void brightness_curve_pq(double scale, uint16_t *curve, size_t size);

#endif
