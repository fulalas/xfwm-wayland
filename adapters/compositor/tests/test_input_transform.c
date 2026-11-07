#include <wlr/util/box.h>

#include "check.h"
#include "compositor/services.h"
#include "input/input_transform.h"

enum {
    CORNER_COUNT = 4,
    OUTPUT_TRANSFORM_COUNT = 8,
};

static const input_point corners[CORNER_COUNT] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

static bool same_point(input_point a, input_point b)
{
    return fabs(a.x - b.x) < 1e-9 && fabs(a.y - b.y) < 1e-9;
}

static bool maps_corners(const input_matrix *matrix, const input_point *expected)
{
    for (int i = 0; i < CORNER_COUNT; i++) {
        if (!same_point(input_matrix_apply(matrix, corners[i]), expected[i])) {
            return false;
        }
    }
    return true;
}

static void test_identity_keeps_points(void)
{
    input_matrix identity = input_matrix_identity();
    input_point point = { 0.25, 0.75 };

    CHECK(same_point(point, input_matrix_apply(&identity, point)));
}

static void test_device_rotation_clockwise(void)
{
    const input_point quarter[] = { { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 0 } };
    const input_point half[] = { { 1, 1 }, { 0, 1 }, { 0, 0 }, { 1, 0 } };
    const input_point three_quarters[] = { { 0, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 } };
    input_matrix m90 = input_matrix_for_device(90, 0);
    input_matrix m180 = input_matrix_for_device(180, 0);
    input_matrix m270 = input_matrix_for_device(270, 0);

    CHECK(maps_corners(&m90, quarter));
    CHECK(maps_corners(&m180, half));
    CHECK(maps_corners(&m270, three_quarters));
}

static void test_device_rotation_rounds_and_wraps(void)
{
    input_matrix exact = input_matrix_for_device(90, 0);
    input_matrix rounded = input_matrix_for_device(100, 0);
    input_matrix negative = input_matrix_for_device(-270, 0);
    input_matrix wrapped = input_matrix_for_device(450, 0);
    input_matrix nearly_full = input_matrix_for_device(350, 0);
    input_matrix identity = input_matrix_identity();
    const input_point quarter[] = { { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 0 } };

    CHECK(maps_corners(&exact, quarter));
    CHECK(maps_corners(&rounded, quarter));
    CHECK(maps_corners(&negative, quarter));
    CHECK(maps_corners(&wrapped, quarter));
    CHECK(maps_corners(&nearly_full, corners));
    CHECK(maps_corners(&identity, corners));
}

static void test_device_reflection(void)
{
    input_matrix x = input_matrix_for_device(0, INPUT_REFLECT_X);
    input_matrix y = input_matrix_for_device(0, INPUT_REFLECT_Y);
    input_matrix both = input_matrix_for_device(0, INPUT_REFLECT_X | INPUT_REFLECT_Y);
    input_matrix half = input_matrix_for_device(180, 0);

    CHECK(maps_corners(&x, (const input_point[]) { { 1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 } }));
    CHECK(maps_corners(&y, (const input_point[]) { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } }));
    for (int i = 0; i < CORNER_COUNT; i++) {
        CHECK(same_point(input_matrix_apply(&half, corners[i]),
                         input_matrix_apply(&both, corners[i])));
    }
}

static void test_device_reflects_before_rotating(void)
{
    input_matrix reflect = input_matrix_for_device(0, INPUT_REFLECT_X);
    input_matrix rotate = input_matrix_for_device(90, 0);
    input_matrix combined = input_matrix_for_device(90, INPUT_REFLECT_X);
    input_point point = { 0.2, 0.1 };
    input_point stepwise = input_matrix_apply(&rotate, input_matrix_apply(&reflect, point));

    CHECK(same_point(stepwise, input_matrix_apply(&combined, point)));
}

static void test_then_applies_first_matrix_first(void)
{
    input_matrix rotate = input_matrix_for_device(90, 0);
    input_matrix reflect = input_matrix_for_device(0, INPUT_REFLECT_Y);
    input_matrix combined = input_matrix_then(&rotate, &reflect);
    input_point point = { 0.3, 0.6 };
    input_point stepwise = input_matrix_apply(&reflect, input_matrix_apply(&rotate, point));

    CHECK(same_point(stepwise, input_matrix_apply(&combined, point)));
}

static void test_output_matches_wlroots_buffer_to_logical(void)
{
    const input_point samples[] = { { 0, 0 }, { 0.25, 0.1 }, { 0.9, 0.6 }, { 1, 1 } };

    for (int transform = 0; transform < OUTPUT_TRANSFORM_COUNT; transform++) {
        input_matrix matrix = input_matrix_for_output(transform);
        for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
            struct wlr_fbox box = { .x = samples[i].x, .y = samples[i].y };
            wlr_fbox_transform(&box, &box, (enum wl_output_transform) transform, 1, 1);
            input_point expected = { box.x, box.y };
            CHECK(same_point(expected, input_matrix_apply(&matrix, samples[i])));
        }
    }
}

static void test_turn_ignores_translation(void)
{
    input_matrix rotate = input_matrix_for_device(90, 0);
    input_matrix reflect = input_matrix_for_device(0, INPUT_REFLECT_X);
    input_point tilt = { 30, -10 };

    CHECK(same_point((input_point) { 10, 30 }, input_matrix_turn(&rotate, tilt)));
    CHECK(same_point((input_point) { -30, -10 }, input_matrix_turn(&reflect, tilt)));
}

int main(void)
{
    static const check_case cases[] = {
        { "identity_keeps_points", test_identity_keeps_points },
        { "device_rotation_clockwise", test_device_rotation_clockwise },
        { "device_rotation_rounds_and_wraps", test_device_rotation_rounds_and_wraps },
        { "device_reflection", test_device_reflection },
        { "device_reflects_before_rotating", test_device_reflects_before_rotating },
        { "then_applies_first_matrix_first", test_then_applies_first_matrix_first },
        { "output_matches_wlroots_buffer_to_logical",
          test_output_matches_wlroots_buffer_to_logical },
        { "turn_ignores_translation", test_turn_ignores_translation },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
