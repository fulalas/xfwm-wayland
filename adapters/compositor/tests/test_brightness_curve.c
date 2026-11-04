#include "brightness_curve.h"
#include "check.h"

enum { CURVE_SIZE = 4096 };

static void test_neutral_scale_keeps_signal(void)
{
    uint16_t curve[CURVE_SIZE];
    brightness_curve_pq(1.0, curve, CURVE_SIZE);
    for (int i = 0; i < CURVE_SIZE; i += 512) {
        CHECK_DOUBLE_NEAR(i * 65535.0 / (CURVE_SIZE - 1), curve[i], 2.0);
    }
}

static void test_half_light_lowers_peak(void)
{
    uint16_t curve[CURVE_SIZE];
    brightness_curve_pq(0.5, curve, CURVE_SIZE);
    CHECK_INT_EQ(0, curve[0]);
    CHECK_DOUBLE_NEAR(60721.0, curve[CURVE_SIZE - 1], 2.0);
}

static void test_brighter_curve_rises_and_clips(void)
{
    uint16_t curve[CURVE_SIZE];
    brightness_curve_pq(2.0, curve, CURVE_SIZE);
    for (int i = 1; i < CURVE_SIZE; i++) {
        CHECK(curve[i] >= curve[i - 1]);
    }
    CHECK_INT_EQ(65535, curve[CURVE_SIZE - 1]);
    CHECK(curve[CURVE_SIZE / 2] > CURVE_SIZE / 2 * 65535 / (CURVE_SIZE - 1));
}

int main(void)
{
    const check_case cases[] = {
        {"neutral_scale_keeps_signal", test_neutral_scale_keeps_signal},
        {"half_light_lowers_peak", test_half_light_lowers_peak},
        {"brighter_curve_rises_and_clips", test_brighter_curve_rises_and_clips},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
