#include "canvas.h"
#include "check.h"
#include "fixture.h"

static const double fractional_scale = 1.32;
static const wm_size title_band = { 29, 29 };

static void test_fractional_scale_fills_last_pixel(void)
{
    canvas canvas = appearance_canvas_create(title_band, fractional_scale);
    cairo_set_source_rgb(canvas.cr, 1.0, 1.0, 1.0);
    cairo_rectangle(canvas.cr, 0, 0, title_band.width, title_band.height);
    cairo_fill(canvas.cr);
    cairo_surface_t *surface = appearance_canvas_finish(&canvas);
    int last_column = cairo_image_surface_get_width(surface) - 1;
    int last_row = cairo_image_surface_get_height(surface) - 1;

    CHECK_INT_EQ(255, fixture_alpha_at(surface, last_column, last_row));
    cairo_surface_destroy(surface);
}

int main(void)
{
    static const check_case cases[] = {
        { "fractional_scale_fills_last_pixel", test_fractional_scale_fills_last_pixel },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
