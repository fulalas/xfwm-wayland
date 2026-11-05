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
    frame_paint_request request = { &layout, { 5, 0, 10, 5 }, 1.5, "Title", NULL };
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
    frame_paint_request request = { &layout, { 0, 0, 2, 4 }, 1.5, NULL, NULL };
    cairo_surface_t *surface = painter.paint_frame(painter.self, &request);

    CHECK_INT_EQ(3, cairo_image_surface_get_width(surface));
    CHECK_INT_EQ(255, fixture_alpha_at(surface, 2, 2));
    cairo_surface_destroy(surface);
    appearance_destroy(appearance);
    fixture_dir_remove(dir);
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

static uint32_t pixel_at(cairo_surface_t *surface, int x, int y)
{
    const unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

    return ((const uint32_t *) (const void *) (data + y * stride))[x];
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
        { "band_edge_stays_opaque", test_band_edge_stays_opaque },
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
