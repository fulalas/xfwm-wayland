#include "check.h"
#include "frame_bands.h"

static wm_decor_layout layout_with(wm_extents extents, wm_size frame)
{
    wm_decor_layout layout = {0};
    layout.has_frame = true;
    layout.extents = extents;
    layout.frame_size = frame;
    return layout;
}

static void test_bands_cover_frame_edges(void)
{
    wm_decor_layout layout = layout_with((wm_extents){4, 5, 24, 6}, (wm_size){209, 230});
    frame_bands bands = frame_bands_compute(&layout);
    CHECK_RECT_EQ(((wm_rect){0, 0, 209, 24}), bands.rects[FRAME_BAND_TOP]);
    CHECK_RECT_EQ(((wm_rect){0, 224, 209, 6}), bands.rects[FRAME_BAND_BOTTOM]);
    CHECK_RECT_EQ(((wm_rect){0, 24, 4, 200}), bands.rects[FRAME_BAND_LEFT]);
    CHECK_RECT_EQ(((wm_rect){204, 24, 5, 200}), bands.rects[FRAME_BAND_RIGHT]);
}

static void test_shaded_frame_has_empty_sides(void)
{
    wm_decor_layout layout = layout_with((wm_extents){4, 4, 24, 4}, (wm_size){100, 28});
    frame_bands bands = frame_bands_compute(&layout);
    CHECK(wm_rect_is_empty(&bands.rects[FRAME_BAND_LEFT]));
    CHECK(wm_rect_is_empty(&bands.rects[FRAME_BAND_RIGHT]));
    CHECK(!wm_rect_is_empty(&bands.rects[FRAME_BAND_TOP]));
}

static void test_borderless_sides_are_hidden(void)
{
    wm_decor_layout layout = layout_with((wm_extents){0, 0, 20, 0}, (wm_size){100, 120});
    frame_bands bands = frame_bands_compute(&layout);
    CHECK(!wm_rect_is_empty(&bands.rects[FRAME_BAND_TOP]));
    CHECK(wm_rect_is_empty(&bands.rects[FRAME_BAND_BOTTOM]));
    CHECK(wm_rect_is_empty(&bands.rects[FRAME_BAND_LEFT]));
}

static void test_overlapping_extents_clamp_side_height(void)
{
    wm_decor_layout layout = layout_with((wm_extents){2, 2, 30, 30}, (wm_size){50, 40});
    frame_bands bands = frame_bands_compute(&layout);
    CHECK_INT_EQ(0, bands.rects[FRAME_BAND_LEFT].height);
}

int main(void)
{
    const check_case cases[] = {
        {"bands_cover_frame_edges", test_bands_cover_frame_edges},
        {"shaded_frame_has_empty_sides", test_shaded_frame_has_empty_sides},
        {"borderless_sides_are_hidden", test_borderless_sides_are_hidden},
        {"overlapping_extents_clamp_side_height", test_overlapping_extents_clamp_side_height},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
