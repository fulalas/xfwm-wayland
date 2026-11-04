#include <stdint.h>

#include "appearance/appearance.h"
#include "check.h"

typedef struct menu_fixture {
    appearance *appearance;
    compositor_osd_painter painter;
    wm_menu_level level;
    menu_layout layout;
} menu_fixture;

static menu_fixture fixture_create(void)
{
    menu_fixture fixture = { .appearance = appearance_create() };

    fixture.painter = appearance_painter(fixture.appearance).osd;
    fixture.level = (wm_menu_level) { .count = 3, .selected = 2 };
    fixture.level.items[0] = (wm_menu_item) { .label = WM_MENU_MOVE, .sensitive = true };
    fixture.level.items[1] = (wm_menu_item) { .label = WM_MENU_SEPARATOR };
    fixture.level.items[2] = (wm_menu_item) { .label = WM_MENU_CLOSE, .sensitive = true };
    fixture.layout = fixture.painter.layout_menu(fixture.painter.self, &fixture.level);
    return fixture;
}

static cairo_surface_t *paint(const menu_fixture *fixture)
{
    menu_paint_request request = { &fixture->level, &fixture->layout, 1.0 };

    return fixture->painter.paint_menu(fixture->painter.self, &request);
}

static uint32_t pixel_at(cairo_surface_t *surface, int x, int y)
{
    const unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

    return ((const uint32_t *) (const void *) (data + y * stride))[x];
}

static void test_rows_stack_inside_panel(void)
{
    menu_fixture fixture = fixture_create();
    const wm_rect *items = fixture.layout.items;

    CHECK_INT_EQ(3, fixture.layout.count);
    CHECK_INT_EQ(items[0].y + items[0].height, items[1].y);
    CHECK_INT_EQ(items[1].y + items[1].height, items[2].y);
    CHECK(items[1].height < items[0].height);
    CHECK(wm_rect_bottom(&items[2]) < fixture.layout.size.height);
    CHECK(items[0].y > 0 && items[0].x > 0);
    CHECK(wm_rect_right(&items[0]) < fixture.layout.size.width);
    appearance_destroy(fixture.appearance);
}

static void test_selected_row_is_painted_on_its_rect(void)
{
    menu_fixture fixture = fixture_create();
    wm_rect row = fixture.layout.items[2];
    cairo_surface_t *surface = paint(&fixture);
    int x = row.x + 1;
    uint32_t selected = pixel_at(surface, x, row.y);

    CHECK_INT_EQ(selected, pixel_at(surface, x, wm_rect_bottom(&row) - 1));
    CHECK(pixel_at(surface, x, row.y - 1) != selected);
    CHECK(pixel_at(surface, x, wm_rect_bottom(&row)) != selected);
    CHECK(pixel_at(surface, x, fixture.layout.items[0].y) != selected);
    cairo_surface_destroy(surface);
    appearance_destroy(fixture.appearance);
}

static void test_separator_is_painted_inside_its_rect(void)
{
    menu_fixture fixture = fixture_create();
    wm_rect row = fixture.layout.items[1];
    cairo_surface_t *surface = paint(&fixture);
    int x = row.x + 1;

    CHECK(pixel_at(surface, x, row.y + row.height / 2) != pixel_at(surface, x, row.y));
    cairo_surface_destroy(surface);
    appearance_destroy(fixture.appearance);
}

static void test_layout_is_capped_at_item_max(void)
{
    menu_fixture fixture = fixture_create();
    wm_menu_level level = { .count = WM_MENU_ITEM_MAX + 5, .selected = -1 };
    menu_layout layout = fixture.painter.layout_menu(fixture.painter.self, &level);

    CHECK_INT_EQ(WM_MENU_ITEM_MAX, layout.count);
    CHECK(wm_rect_bottom(&layout.items[WM_MENU_ITEM_MAX - 1]) < layout.size.height);
    appearance_destroy(fixture.appearance);
}

int main(void)
{
    static const check_case cases[] = {
        { "rows_stack_inside_panel", test_rows_stack_inside_panel },
        { "selected_row_is_painted_on_its_rect", test_selected_row_is_painted_on_its_rect },
        { "separator_is_painted_inside_its_rect", test_separator_is_painted_inside_its_rect },
        { "layout_is_capped_at_item_max", test_layout_is_capped_at_item_max },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
