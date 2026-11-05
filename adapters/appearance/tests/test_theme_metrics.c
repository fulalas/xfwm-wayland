#include <string.h>

#include <glib.h>

#include "appearance/appearance.h"
#include "check.h"
#include "fixture.h"
#include "theme.h"
#include "theme_dir.h"

static const char themerc_content[] = "button_offset=3\ntitle_shadow_active=frame\n";
static const char big_title_themerc[] = "title_font=Sans 40\n";

static const char left_xpm[] = "/* XPM */\nstatic char *l[] = {\n\"4 10 1 1\",\n"
                               "\". c #000000 s active_border_color\",\n"
                               "\"....\",\"....\",\"....\",\"....\",\"....\",\n"
                               "\"....\",\"....\",\"....\",\"....\",\"....\"};\n";

static const char title_xpm[] = "/* XPM */\nstatic char *t[] = {\n\"2 6 1 1\",\n"
                                "\". c #C0C0FF s active_color_1\",\n"
                                "\"..\",\"..\",\"..\",\"..\",\"..\",\"..\"};\n";

static char *make_theme_with_rc(const char *themerc)
{
    char *dir = fixture_dir_create();
    char *paths[] = {
        fixture_write(dir, "themerc", themerc),
        fixture_write(dir, "left-active.xpm", left_xpm),
        fixture_write(dir, "title-1-active.xpm", title_xpm),
        fixture_write(dir, "title-active-stretch.xpm", title_xpm),
        fixture_write(dir, "top-left-inactive.xpm", title_xpm),
    };

    for (size_t i = 0; i < G_N_ELEMENTS(paths); i++) {
        g_free(paths[i]);
    }
    fixture_write_png(dir, "close-active.png", 16, 14);
    fixture_write_png(dir, "top-left-inactive.png", 8, 8);
    return dir;
}

static char *make_theme(void)
{
    return make_theme_with_rc(themerc_content);
}

static wm_theme_metrics load_metrics(appearance *appearance, const char *dir)
{
    ui_style style = appearance_fallback_style();
    theme_request request = { dir, "Sans 9", 1.0, &style };

    CHECK(appearance_load(appearance, &request));
    return appearance_metrics(appearance);
}

static bool size_is(wm_size size, int width, int height)
{
    return size.width == width && size.height == height;
}

static void test_metrics_from_pixmaps(void)
{
    char *dir = make_theme();
    appearance *appearance = appearance_create();
    wm_theme_metrics metrics = load_metrics(appearance, dir);

    CHECK(size_is(metrics.side[WM_SIDE_LEFT][WM_FRAME_ACTIVE], 4, 10));
    CHECK(size_is(metrics.title[0][WM_FRAME_ACTIVE], 2, 6));
    CHECK(size_is(metrics.side_stretch[WM_SIDE_TOP][WM_FRAME_ACTIVE], 2, 6));
    CHECK(size_is(metrics.button[WM_BUTTON_CLOSE][WM_IMAGE_ACTIVE], 16, 14));
    appearance_destroy(appearance);
    fixture_dir_remove(dir);
}

static void test_missing_pixmaps_are_empty(void)
{
    char *dir = make_theme();
    appearance *appearance = appearance_create();
    wm_theme_metrics metrics = load_metrics(appearance, dir);

    CHECK(size_is(metrics.side[WM_SIDE_RIGHT][WM_FRAME_ACTIVE], 0, 0));
    CHECK(size_is(metrics.side[WM_SIDE_TOP][WM_FRAME_ACTIVE], 0, 0));
    CHECK(size_is(metrics.button[WM_BUTTON_MENU][WM_IMAGE_TOGGLED_PRESSED], 0, 0));
    CHECK(size_is(metrics.top[4][WM_FRAME_INACTIVE], 0, 0));
    appearance_destroy(appearance);
    fixture_dir_remove(dir);
}

static void test_overlay_composes_on_xpm_size(void)
{
    char *dir = make_theme();
    appearance *appearance = appearance_create();
    wm_theme_metrics metrics = load_metrics(appearance, dir);

    CHECK(size_is(metrics.corner[WM_CORNER_TOP_LEFT][WM_FRAME_INACTIVE], 2, 6));
    appearance_destroy(appearance);
    fixture_dir_remove(dir);
}

static void test_theme_options_are_raw_values(void)
{
    char *dir = make_theme();
    appearance *appearance = appearance_create();

    load_metrics(appearance, dir);
    CHECK(strcmp(appearance_theme_option(appearance, "button_offset"), "3") == 0);
    CHECK(strcmp(appearance_theme_option(appearance, "TITLE_SHADOW_ACTIVE"), "frame") == 0);
    CHECK(appearance_theme_option(appearance, "button_spacing") == NULL);
    appearance_destroy(appearance);
    fixture_dir_remove(dir);
}

static void test_same_request_is_loaded(void)
{
    char *dir = make_theme();
    appearance *appearance = appearance_create();
    ui_style style = appearance_fallback_style();
    theme_request request = { dir, "Sans 9", 1.0, &style };

    CHECK(!appearance_is_loaded(appearance, &request));
    CHECK(appearance_load(appearance, &request));
    CHECK(appearance_is_loaded(appearance, &request));
    request.max_output_scale = 2.0;
    CHECK(!appearance_is_loaded(appearance, &request));
    request.max_output_scale = 1.0;
    g_strlcpy(style.font, "Serif 12", sizeof style.font);
    CHECK(!appearance_is_loaded(appearance, &request));
    appearance_destroy(appearance);
    fixture_dir_remove(dir);
}

static int title_height(const char *themerc)
{
    char *dir = make_theme_with_rc(themerc);
    appearance *appearance = appearance_create();
    compositor_painter painter = appearance_painter(appearance);
    ui_style style = appearance_fallback_style();
    theme_request request = { dir, "Sans 6", 1.0, &style };

    CHECK(appearance_load(appearance, &request));
    int height = painter.measure_title(painter.self, "Title").height;
    appearance_destroy(appearance);
    fixture_dir_remove(dir);
    return height;
}

static void test_themerc_title_font_wins(void)
{
    CHECK(title_height(big_title_themerc) > 2 * title_height(themerc_content));
}

static void test_double_scale_halves_metrics(void)
{
    char *dir = make_theme();
    theme_source source = { { dir, NULL }, 2 };
    theme *theme = appearance_theme_create(&source);
    wm_theme_metrics metrics = appearance_theme_metrics(theme);

    CHECK(size_is(metrics.side[WM_SIDE_LEFT][WM_FRAME_ACTIVE], 2, 5));
    CHECK(size_is(metrics.button[WM_BUTTON_CLOSE][WM_IMAGE_ACTIVE], 8, 7));
    appearance_theme_destroy(theme);
    fixture_dir_remove(dir);
}

static const char clear_pixel_xpm[] = "/* XPM */\nstatic char *c[] = {\n\"1 1 1 1\",\n"
                                      "\"  c None\",\n\" \"};\n";

static void test_half_transparent_pixels_are_cut_out(void)
{
    enum { HALF_BLACK = 0x00000080 };
    char *dir = fixture_dir_create();
    g_free(fixture_write(dir, "top-left-active.xpm", clear_pixel_xpm));
    CHECK(fixture_write_png_pixel(dir, "top-left-active.png", HALF_BLACK));
    theme_image_source source = { dir, NULL };
    cairo_surface_t *image = appearance_theme_image_load(&source, "top-left-active");

    CHECK(image != NULL);
    CHECK_INT_EQ(0, fixture_alpha_at(image, 0, 0));
    cairo_surface_destroy(image);
    fixture_dir_remove(dir);
}

static void test_absolute_name_is_the_theme_dir(void)
{
    char *dir = make_theme();
    theme_location location = appearance_theme_locate(dir, 2.0);

    CHECK(strcmp(location.dir, dir) == 0);
    CHECK_INT_EQ(1, location.pixmap_scale);
    appearance_theme_location_clear(&location);
    fixture_dir_remove(dir);
}

int main(void)
{
    static const check_case cases[] = {
        { "metrics_from_pixmaps", test_metrics_from_pixmaps },
        { "missing_pixmaps_are_empty", test_missing_pixmaps_are_empty },
        { "overlay_composes_on_xpm_size", test_overlay_composes_on_xpm_size },
        { "theme_options_are_raw_values", test_theme_options_are_raw_values },
        { "same_request_is_loaded", test_same_request_is_loaded },
        { "themerc_title_font_wins", test_themerc_title_font_wins },
        { "double_scale_halves_metrics", test_double_scale_halves_metrics },
        { "half_transparent_pixels_are_cut_out", test_half_transparent_pixels_are_cut_out },
        { "absolute_name_is_the_theme_dir", test_absolute_name_is_the_theme_dir },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
