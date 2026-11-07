#include "check.h"
#include "xwayland/x11_pixmap_icon.h"

static uint32_t pixel(cairo_surface_t *surface, int x, int y)
{
    const uint8_t *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    return ((const uint32_t *) (data + y * stride))[x];
}

static void test_color_pixmap_is_opaque(void)
{
    const uint32_t colors[] = {0x00ff0000, 0x0000ff00};
    x11_icon_pixels pixels = {2, 1, 24, colors, 2, {NULL, 0, true}, NULL};
    cairo_surface_t *surface = x11_icon_from_pixels(&pixels);
    CHECK_INT_EQ(0xffff0000u, pixel(surface, 0, 0));
    CHECK_INT_EQ(0xff00ff00u, pixel(surface, 1, 0));
    cairo_surface_destroy(surface);
}

static void test_mask_hides_pixels(void)
{
    const uint32_t colors[] = {0x00ff0000, 0x0000ff00};
    const uint8_t mask_bits[] = {0x01};
    x11_bitmap mask = {mask_bits, 1, true};
    x11_icon_pixels pixels = {2, 1, 24, colors, 2, {NULL, 0, true}, &mask};
    cairo_surface_t *surface = x11_icon_from_pixels(&pixels);
    CHECK_INT_EQ(0xffff0000u, pixel(surface, 0, 0));
    CHECK_INT_EQ(0, pixel(surface, 1, 0));
    cairo_surface_destroy(surface);
}

static void test_bitmap_icon_is_black_on_white(void)
{
    const uint8_t bits[] = {0x80};
    x11_icon_pixels pixels = {2, 1, 1, NULL, 0, {bits, 1, false}, NULL};
    cairo_surface_t *surface = x11_icon_from_pixels(&pixels);
    CHECK_INT_EQ(0xff000000u, pixel(surface, 0, 0));
    CHECK_INT_EQ(0xffffffffu, pixel(surface, 1, 0));
    cairo_surface_destroy(surface);
}

int main(void)
{
    static const check_case cases[] = {
        {"color_pixmap_is_opaque", test_color_pixmap_is_opaque},
        {"mask_hides_pixels", test_mask_hides_pixels},
        {"bitmap_icon_is_black_on_white", test_bitmap_icon_is_black_on_white},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
