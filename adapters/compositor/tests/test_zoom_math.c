#include "check.h"
#include "zoom_math.h"

static const double TOLERANCE = 1e-9;

static void test_level_one_shows_everything(void)
{
    zoom_request request = {{1920, 1080}, {500, 300}, 1.0};
    zoom_box box = zoom_source_box(&request);
    CHECK_DOUBLE_NEAR(0.0, box.x, TOLERANCE);
    CHECK_DOUBLE_NEAR(0.0, box.y, TOLERANCE);
    CHECK_DOUBLE_NEAR(1920.0, box.width, TOLERANCE);
    CHECK_DOUBLE_NEAR(1080.0, box.height, TOLERANCE);
}

static void test_center_stays_fixed(void)
{
    zoom_request request = {{1000, 800}, {400, 200}, 2.0};
    zoom_box box = zoom_source_box(&request);
    CHECK_DOUBLE_NEAR(200.0, box.x, TOLERANCE);
    CHECK_DOUBLE_NEAR(100.0, box.y, TOLERANCE);
    CHECK_DOUBLE_NEAR(500.0, box.width, TOLERANCE);
    double mapped_x = (request.center.x - box.x) * request.level;
    CHECK_DOUBLE_NEAR(400.0, mapped_x, TOLERANCE);
}

static void test_corner_center_keeps_box_inside(void)
{
    zoom_request request = {{1000, 800}, {1000, 800}, 4.0};
    zoom_box box = zoom_source_box(&request);
    CHECK_DOUBLE_NEAR(1000.0, box.x + box.width, TOLERANCE);
    CHECK_DOUBLE_NEAR(800.0, box.y + box.height, TOLERANCE);
}

static void test_center_outside_is_clamped(void)
{
    zoom_request request = {{1000, 800}, {-50, 5000}, 2.0};
    zoom_box box = zoom_source_box(&request);
    CHECK_DOUBLE_NEAR(0.0, box.x, TOLERANCE);
    CHECK_DOUBLE_NEAR(400.0, box.y, TOLERANCE);
}

int main(void)
{
    const check_case cases[] = {
        {"level_one_shows_everything", test_level_one_shows_everything},
        {"center_stays_fixed", test_center_stays_fixed},
        {"corner_center_keeps_box_inside", test_corner_center_keeps_box_inside},
        {"center_outside_is_clamped", test_center_outside_is_clamped},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
