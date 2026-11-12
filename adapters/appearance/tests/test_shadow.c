#include <stdint.h>

#include "check.h"
#include "fixture.h"
#include "shadow.h"
#include "wm/shadow.h"


static void test_template_size(void)
{
    shadow_template shadow = appearance_shadow_template_create();

    CHECK_INT_EQ(wm_shadow_kernel_size(), shadow.corner_size);
    CHECK_INT_EQ(3 * shadow.corner_size, cairo_image_surface_get_width(shadow.surface));
    CHECK_INT_EQ(3 * shadow.corner_size, cairo_image_surface_get_height(shadow.surface));
    CHECK(cairo_image_surface_get_format(shadow.surface) == CAIRO_FORMAT_ARGB32);
    cairo_surface_destroy(shadow.surface);
}

static void test_template_is_symmetric(void)
{
    shadow_template shadow = appearance_shadow_template_create();
    int size = cairo_image_surface_get_width(shadow.surface);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            unsigned value = fixture_alpha_at(shadow.surface, x, y);
            CHECK_INT_EQ(value, fixture_alpha_at(shadow.surface, size - 1 - x, y));
            CHECK_INT_EQ(value, fixture_alpha_at(shadow.surface, x, size - 1 - y));
        }
    }
    cairo_surface_destroy(shadow.surface);
}

static void test_edges_are_constant_in_middle(void)
{
    shadow_template shadow = appearance_shadow_template_create();
    int corner = shadow.corner_size;

    for (int depth = 0; depth < corner; depth++) {
        for (int along = corner; along < 2 * corner; along++) {
            CHECK_INT_EQ(fixture_alpha_at(shadow.surface, corner, depth),
                         fixture_alpha_at(shadow.surface, along, depth));
            CHECK_INT_EQ(fixture_alpha_at(shadow.surface, depth, corner),
                         fixture_alpha_at(shadow.surface, depth, along));
        }
    }
    cairo_surface_destroy(shadow.surface);
}

static void test_gaussian_falls_off_outward(void)
{
    shadow_template shadow = appearance_shadow_template_create();
    int corner = shadow.corner_size;

    CHECK(fixture_alpha_at(shadow.surface, corner, corner) >= 250);
    CHECK_INT_EQ(0, fixture_alpha_at(shadow.surface, 0, 0));
    for (int depth = 1; depth < corner; depth++) {
        unsigned outer = fixture_alpha_at(shadow.surface, corner, depth - 1);
        CHECK(fixture_alpha_at(shadow.surface, corner, depth) >= outer);
    }
    cairo_surface_destroy(shadow.surface);
}

int main(void)
{
    static const check_case cases[] = {
        { "template_size", test_template_size },
        { "template_is_symmetric", test_template_is_symmetric },
        { "edges_are_constant_in_middle", test_edges_are_constant_in_middle },
        { "gaussian_falls_off_outward", test_gaussian_falls_off_outward },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
