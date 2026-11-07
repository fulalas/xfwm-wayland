#include "brightness_curve.h"

#include <math.h>

static const double pq_m1 = 2610.0 / 16384.0;
static const double pq_m2 = 2523.0 / 4096.0 * 128.0;
static const double pq_c1 = 3424.0 / 4096.0;
static const double pq_c2 = 2413.0 / 4096.0 * 32.0;
static const double pq_c3 = 2392.0 / 4096.0 * 32.0;
static const double curve_max = 65535.0;

static double pq_to_light(double signal);
static double light_to_pq(double light);

void brightness_curve_pq(double scale, uint16_t *curve, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        double signal = size > 1 ? (double) i / (double) (size - 1) : 0.0;
        double light = fmin(1.0, scale * pq_to_light(signal));
        curve[i] = (uint16_t) lround(light_to_pq(light) * curve_max);
    }
}

static double pq_to_light(double signal)
{
    double p = pow(signal, 1.0 / pq_m2);
    return pow(fmax(p - pq_c1, 0.0) / (pq_c2 - pq_c3 * p), 1.0 / pq_m1);
}

static double light_to_pq(double light)
{
    double l = pow(light, pq_m1);
    return pow((pq_c1 + pq_c2 * l) / (1.0 + pq_c3 * l), pq_m2);
}
