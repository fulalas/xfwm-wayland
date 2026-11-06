#include <glib.h>
#include <stdint.h>

#include "appearance/appearance.h"
#include "check.h"
#include "fixture.h"

static wm_cycle_view grid_view(size_t count)
{
    wm_cycle_view view = {
        .mode = WM_TABWIN_GRID,
        .columns = 3,
        .rows = 2,
        .item_size = 48,
        .monitor = { 0, 0, 1920, 1080 },
        .count = count,
        .selected = 1,
    };

    for (size_t i = 0; i < count; i++) {
        view.items[i].title = "Terminal";
    }
    return view;
}

static void test_frame_surface_uses_scaled_band(void)
{
    appearance *appearance = appearance_create();
    compositor_painter painter = appearance_painter(appearance);
    wm_decor_layout layout = { .has_frame = true };
    frame_paint_request request = { &layout, { 5, 0, 10, 5 }, 1.5, "Title", NULL, { 0, 0 } };
    cairo_surface_t *surface = painter.paint_frame(painter.self, &request);

    CHECK_INT_EQ(15, cairo_image_surface_get_width(surface));
    CHECK_INT_EQ(8, cairo_image_surface_get_height(surface));
    CHECK(cairo_image_surface_get_format(surface) == CAIRO_FORMAT_ARGB32);
    cairo_surface_destroy(surface);
    appearance_destroy(appearance);
}

static const char half_corner_xpm[] = "/* XPM */\nstatic char *c[] = {\n"
                                      "\"4 4 2 1\",\n\". c #FF0000\",\n\"  c None\",\n"
                                      "\"..  \",\"..  \",\"..  \",\"..  \"};\n";

static void test_band_edge_stays_opaque(void)
{
    char *dir = fixture_dir_create();
    g_free(fixture_write(dir, "themerc", ""));
    g_free(fixture_write(dir, "bottom-left-active.xpm", half_corner_xpm));
    appearance *appearance = appearance_create();
    ui_style style = appearance_fallback_style();
    theme_request theme = { dir, "Sans 9", 1.0, &style };
    CHECK(appearance_load(appearance, &theme));
    compositor_painter painter = appearance_painter(appearance);
    wm_decor_layout layout = { .has_frame = true, .op_count = 1 };
    layout.ops[0] = (wm_paint_op) {
        .pixmap = { WM_PIXMAP_CORNER, WM_CORNER_BOTTOM_LEFT, WM_FRAME_ACTIVE },
        .dest = { 0, 0, 4, 4 },
        .mode = WM_FILL_PLAIN,
        .clip = { 0, 0, 4, 4 },
    };
    frame_paint_request request = { &layout, { 0, 0, 2, 4 }, 1.5, NULL, NULL, { 0, 0 } };
    cairo_surface_t *surface = painter.paint_frame(painter.self, &request);

    CHECK_INT_EQ(3, cairo_image_surface_get_width(surface));
    CHECK_INT_EQ(255, fixture_alpha_at(surface, 2, 2));
    cairo_surface_destroy(surface);
    appearance_destroy(appearance);
    fixture_dir_remove(dir);
}

static uint32_t pixel_at(cairo_surface_t *surface, int x, int y)
{
    const unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

    return ((const uint32_t *) (const void *) (data + y * stride))[x];
}

static const char right_corner_xpm[] = "/* XPM */\nstatic char *c[] = {\n"
                                       "\"8 3 2 1\",\n\". c #FF0000\",\n\"# c #0000FF\",\n"
                                       "\".......#\",\".......#\",\".......#\"};\n";

static void check_right_corner(wm_point origin, int last_column)
{
    char *dir = fixture_dir_create();
    g_free(fixture_write(dir, "themerc", ""));
    g_free(fixture_write(dir, "top-right-active.xpm", right_corner_xpm));
    appearance *appearance = appearance_create();
    ui_style style = appearance_fallback_style();
    theme_request theme = { dir, "Sans 9", 1.5, &style };
    CHECK(appearance_load(appearance, &theme));
    compositor_painter painter = appearance_painter(appearance);
    wm_decor_layout layout = { .has_frame = true, .op_count = 1 };
    layout.ops[0] = (wm_paint_op) {
        .pixmap = { WM_PIXMAP_CORNER, WM_CORNER_TOP_RIGHT, WM_FRAME_ACTIVE },
        .dest = { 0, 0, 5, 2 },
        .mode = WM_FILL_PLAIN,
        .clip = { 0, 0, 5, 2 },
    };
    frame_paint_request request = { &layout, { 0, 0, 5, 2 }, 1.5, NULL, NULL, origin };
    cairo_surface_t *surface = painter.paint_frame(painter.self, &request);

    CHECK_INT_EQ(last_column + 1, cairo_image_surface_get_width(surface));
    CHECK_INT_EQ(0xff0000ff, pixel_at(surface, last_column, 1));
    CHECK(pixel_at(surface, last_column - 1, 1) != 0xff0000ff);
    cairo_surface_destroy(surface);
    appearance_destroy(appearance);
    fixture_dir_remove(dir);
}

static void test_right_corner_keeps_its_outer_column(void)
{
    check_right_corner((wm_point) { 0, 0 }, 7);
}

static void test_right_corner_follows_screen_alignment(void)
{
    check_right_corner((wm_point) { 1, 0 }, 6);
}

static const char right_corner_svg[] =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 8 3\" shape-rendering=\"crispEdges\">"
    "<rect width=\"7\" height=\"3\" fill=\"#ff0000\"/>"
    "<rect x=\"7\" width=\"1\" height=\"3\" fill=\"#0000ff\"/></svg>\n";

static int svg_corner_blue_columns(wm_point origin)
{
    char *dir = fixture_dir_create();
    g_free(fixture_write(dir, "themerc", ""));
    g_free(fixture_write(dir, "top-right-active.svg", right_corner_svg));
    appearance *appearance = appearance_create();
    ui_style style = appearance_fallback_style();
    theme_request theme = { dir, "Sans 9", 1.5, &style };
    CHECK(appearance_load(appearance, &theme));
    compositor_painter painter = appearance_painter(appearance);
    wm_decor_layout layout = { .has_frame = true, .op_count = 1 };
    layout.ops[0] = (wm_paint_op) {
        .pixmap = { WM_PIXMAP_CORNER, WM_CORNER_TOP_RIGHT, WM_FRAME_ACTIVE },
        .dest = { 0, 0, 5, 3 },
        .mode = WM_FILL_PLAIN,
        .clip = { 0, 0, 5, 3 },
    };
    frame_paint_request request = { &layout, { 0, 0, 5, 3 }, 1.5, NULL, NULL, origin };
    cairo_surface_t *surface = painter.paint_frame(painter.self, &request);
    int width = cairo_image_surface_get_width(surface);
    int blue = 0;

    CHECK_INT_EQ(0xff0000ff, pixel_at(surface, width - 1, 1));
    for (int x = 0; x < width; x++) {
        blue += pixel_at(surface, x, 1) == 0xff0000ff;
    }
    cairo_surface_destroy(surface);
    appearance_destroy(appearance);
    fixture_dir_remove(dir);
    return blue;
}

static void test_svg_border_keeps_its_width_when_moved(void)
{
    CHECK_INT_EQ(svg_corner_blue_columns((wm_point) { 0, 0 }),
                 svg_corner_blue_columns((wm_point) { 1, 0 }));
}

static const char dark_light_side_xpm[] = "/* XPM */\nstatic char *s[] = {\n"
                                          "\"2 2 2 1\",\n\". c #000000\",\n\"# c #FFFFFF\",\n"
                                          "\".#\",\".#\"};\n";

static void test_tiled_edge_does_not_wrap(void)
{
    enum { OPAQUE_WHITE = 0xFFFFFFFFu };
    char *dir = fixture_dir_create();
    g_free(fixture_write(dir, "themerc", ""));
    g_free(fixture_write(dir, "left-active.xpm", dark_light_side_xpm));
    appearance *appearance = appearance_create();
    ui_style style = appearance_fallback_style();
    theme_request theme = { dir, "Sans 9", 1.0, &style };
    CHECK(appearance_load(appearance, &theme));
    compositor_painter painter = appearance_painter(appearance);
    wm_decor_layout layout = { .has_frame = true, .op_count = 1 };
    layout.ops[0] = (wm_paint_op) {
        .pixmap = { WM_PIXMAP_SIDE, WM_SIDE_LEFT, WM_FRAME_ACTIVE },
        .dest = { 0, 0, 2, 4 },
        .mode = WM_FILL_TILE,
        .clip = { 0, 0, 2, 4 },
    };
    frame_paint_request request = { &layout, { 0, 0, 2, 4 }, 1.5, NULL, NULL, { 0, 0 } };
    cairo_surface_t *surface = painter.paint_frame(painter.self, &request);

    CHECK_INT_EQ(OPAQUE_WHITE, pixel_at(surface, 2, 2));
    cairo_surface_destroy(surface);
    appearance_destroy(appearance);
    fixture_dir_remove(dir);
}

static const char three_color_side_xpm[] = "/* XPM */\nstatic char *s[] = {\n"
                                           "\"3 1 3 1\",\n\"a c #000000\",\n"
                                           "\"b c #FF0000\",\n\"c c #00FF00\",\n"
                                           "\"abc\"};\n";

static cairo_surface_t *paint_tile(const char *xpm, wm_rect band)
{
    char *dir = fixture_dir_create();
    g_free(fixture_write(dir, "themerc", ""));
    g_free(fixture_write(dir, "left-active.xpm", xpm));
    appearance *appearance = appearance_create();
    ui_style style = appearance_fallback_style();
    theme_request theme = { dir, "Sans 9", 1.0, &style };
    CHECK(appearance_load(appearance, &theme));
    compositor_painter painter = appearance_painter(appearance);
    wm_decor_layout layout = { .has_frame = true, .op_count = 1 };
    layout.ops[0] = (wm_paint_op) {
        .pixmap = { WM_PIXMAP_SIDE, WM_SIDE_LEFT, WM_FRAME_ACTIVE },
        .dest = { 0, 0, 6, 1 },
        .origin = { 1, 0 },
        .mode = WM_FILL_TILE,
        .clip = { 0, 0, 6, 1 },
    };
    frame_paint_request request = { &layout, band, 1.0, NULL, NULL, { 0, 0 } };
    cairo_surface_t *surface = painter.paint_frame(painter.self, &request);

    appearance_destroy(appearance);
    fixture_dir_remove(dir);
    return surface;
}

static void test_tile_starts_at_its_origin(void)
{
    enum { BLACK = 0xFF000000u, RED = 0xFFFF0000u, GREEN = 0xFF00FF00u };
    cairo_surface_t *whole = paint_tile(three_color_side_xpm, (wm_rect) { 0, 0, 6, 1 });
    cairo_surface_t *part = paint_tile(three_color_side_xpm, (wm_rect) { 2, 0, 4, 1 });

    CHECK_INT_EQ(GREEN, pixel_at(whole, 0, 0));
    CHECK_INT_EQ(BLACK, pixel_at(whole, 1, 0));
    CHECK_INT_EQ(RED, pixel_at(whole, 2, 0));
    CHECK_INT_EQ(RED, pixel_at(part, 0, 0));
    CHECK_INT_EQ(GREEN, pixel_at(part, 1, 0));
    cairo_surface_destroy(whole);
    cairo_surface_destroy(part);
}

static void test_title_measure_grows_with_text(void)
{
    appearance *appearance = appearance_create();
    compositor_painter painter = appearance_painter(appearance);
    wm_size short_title = painter.measure_title(painter.self, "ab");
    wm_size long_title = painter.measure_title(painter.self, "abcdefgh");

    CHECK(short_title.height > 0);
    CHECK(long_title.width > short_title.width);
    CHECK_INT_EQ(short_title.height, long_title.height);
    appearance_destroy(appearance);
}

static void test_cycle_surface_matches_size(void)
{
    appearance *appearance = appearance_create();
    compositor_osd_painter painter = appearance_painter(appearance).osd;
    wm_cycle_view view = grid_view(5);
    cycle_layout layout = painter.layout_cycle(painter.self, &view);
    cycle_paint_request request = { &view, &layout, NULL, 2.0 };
    cairo_surface_t *surface = painter.paint_cycle(painter.self, &request);

    CHECK_INT_EQ(2 * layout.size.width, cairo_image_surface_get_width(surface));
    CHECK_INT_EQ(2 * layout.size.height, cairo_image_surface_get_height(surface));
    cairo_surface_destroy(surface);
    appearance_destroy(appearance);
}

static void test_size_label_fits_its_text(void)
{
    appearance *appearance = appearance_create();
    compositor_osd_painter painter = appearance_painter(appearance).osd;
    wm_size small = painter.layout_label(painter.self, "(8x2)");
    wm_size large = painter.layout_label(painter.self, "(180x52)");
    label_paint_request request = { "(180x52)", large, 1.5 };
    cairo_surface_t *surface = painter.paint_label(painter.self, &request);

    CHECK(large.width > small.width);
    CHECK_INT_EQ(small.height, large.height);
    CHECK_INT_EQ((int) (1.5 * large.width + 0.5), cairo_image_surface_get_width(surface));
    cairo_surface_destroy(surface);
    appearance_destroy(appearance);
}

static void test_cycle_cells_follow_grid(void)
{
    appearance *appearance = appearance_create();
    compositor_osd_painter painter = appearance_painter(appearance).osd;
    wm_cycle_view view = grid_view(4);
    cycle_layout layout = painter.layout_cycle(painter.self, &view);
    wm_point last_row = { 25, layout.size.height / 2 + 10 };

    CHECK_INT_EQ(4, layout.count);
    CHECK(wm_rect_contains(&layout.items[0], (wm_point) { 25, 25 }));
    CHECK(wm_rect_contains(&layout.items[3], last_row));
    CHECK(!wm_rect_contains(&layout.items[0], (wm_point) { 1, 1 }));
    CHECK_INT_EQ(layout.items[0].y, layout.items[2].y);
    CHECK(layout.items[1].x > wm_rect_right(&layout.items[0]));
    appearance_destroy(appearance);
}

static void test_selected_cell_is_painted_on_its_rect(void)
{
    appearance *appearance = appearance_create();
    compositor_osd_painter painter = appearance_painter(appearance).osd;
    wm_cycle_view view = grid_view(3);
    cycle_layout layout = painter.layout_cycle(painter.self, &view);
    cycle_paint_request request = { &view, &layout, NULL, 1.0 };
    cairo_surface_t *surface = painter.paint_cycle(painter.self, &request);
    wm_rect cell = layout.items[1];
    int x = cell.x + cell.width / 2;
    uint32_t selected = pixel_at(surface, x, cell.y);

    CHECK(pixel_at(surface, x, cell.y - 1) != selected);
    CHECK_INT_EQ(selected, pixel_at(surface, x, wm_rect_bottom(&cell) - 1));
    CHECK(pixel_at(surface, x - cell.x + layout.items[0].x, cell.y) != selected);
    cairo_surface_destroy(surface);
    appearance_destroy(appearance);
}

static void test_previews_only_in_preview_mode(void)
{
    appearance *appearance = appearance_create();
    compositor_osd_painter painter = appearance_painter(appearance).osd;
    wm_cycle_view view = grid_view(2);
    cycle_layout plain = painter.layout_cycle(painter.self, &view);

    view.preview = true;
    view.item_size = 288;
    cycle_layout layout = painter.layout_cycle(painter.self, &view);
    wm_rect preview = layout.previews[1];
    CHECK(wm_rect_is_empty(&plain.previews[0]));
    CHECK_INT_EQ(288, preview.width);
    CHECK(preview.height > 0 && preview.height < 288);
    CHECK(preview.x > layout.size.width / 2 && wm_rect_right(&preview) < layout.size.width);
    appearance_destroy(appearance);
}

static void test_previews_sit_inside_their_cells(void)
{
    appearance *appearance = appearance_create();
    compositor_osd_painter painter = appearance_painter(appearance).osd;
    wm_cycle_view view = grid_view(5);

    view.preview = true;
    view.item_size = 200;
    cycle_layout layout = painter.layout_cycle(painter.self, &view);
    for (size_t i = 0; i < layout.count; i++) {
        wm_rect inside = wm_rect_intersection(&layout.items[i], &layout.previews[i]);
        CHECK_RECT_EQ(layout.previews[i], inside);
    }
    appearance_destroy(appearance);
}

static void test_list_mode_is_column_major(void)
{
    appearance *appearance = appearance_create();
    compositor_osd_painter painter = appearance_painter(appearance).osd;
    wm_cycle_view view = grid_view(4);

    view.mode = WM_TABWIN_LIST;
    view.item_size = 24;
    view.rows = 2;
    cycle_layout layout = painter.layout_cycle(painter.self, &view);
    wm_size size = layout.size;
    CHECK(wm_rect_contains(&layout.items[1], (wm_point) { 30, size.height - 25 }));
    CHECK(wm_rect_contains(&layout.items[2], (wm_point) { size.width - 30, 25 }));
    appearance_destroy(appearance);
}

static void test_menu_surface_matches_size(void)
{
    appearance *appearance = appearance_create();
    compositor_osd_painter painter = appearance_painter(appearance).osd;
    wm_menu_level level = { .count = 2, .selected = 0 };

    level.items[0] = (wm_menu_item) { .label = WM_MENU_ABOVE, .sensitive = true,
                                      .mark = WM_MENU_MARK_RADIO_ON };
    level.items[1] = (wm_menu_item) { .label = WM_MENU_WORKSPACES, .has_submenu = true };
    menu_layout layout = painter.layout_menu(painter.self, &level);
    menu_paint_request request = { &level, &layout, 1.25 };
    cairo_surface_t *surface = painter.paint_menu(painter.self, &request);
    wm_size size = layout.size;
    CHECK(size.width > 0 && size.height > 0);
    CHECK_INT_EQ((int) ((size.width * 5 + 3) / 4), cairo_image_surface_get_width(surface));
    cairo_surface_destroy(surface);
    appearance_destroy(appearance);
}

int main(void)
{
    static const check_case cases[] = {
        { "frame_surface_uses_scaled_band", test_frame_surface_uses_scaled_band },
        { "size_label_fits_its_text", test_size_label_fits_its_text },
        { "band_edge_stays_opaque", test_band_edge_stays_opaque },
        { "right_corner_keeps_its_outer_column", test_right_corner_keeps_its_outer_column },
        { "right_corner_follows_screen_alignment", test_right_corner_follows_screen_alignment },
        { "svg_border_keeps_its_width_when_moved", test_svg_border_keeps_its_width_when_moved },
        { "tiled_edge_does_not_wrap", test_tiled_edge_does_not_wrap },
        { "tile_starts_at_its_origin", test_tile_starts_at_its_origin },
        { "title_measure_grows_with_text", test_title_measure_grows_with_text },
        { "cycle_surface_matches_size", test_cycle_surface_matches_size },
        { "cycle_cells_follow_grid", test_cycle_cells_follow_grid },
        { "selected_cell_is_painted_on_its_rect", test_selected_cell_is_painted_on_its_rect },
        { "previews_only_in_preview_mode", test_previews_only_in_preview_mode },
        { "previews_sit_inside_their_cells", test_previews_sit_inside_their_cells },
        { "list_mode_is_column_major", test_list_mode_is_column_major },
        { "menu_surface_matches_size", test_menu_surface_matches_size },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
