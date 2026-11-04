#include "decor_fixture.h"

#include <string.h>

#include "wm/config.h"

static const int title_part_widths[WM_TITLE_PART_COUNT] = {4, 3, 5, 3, 4};

static void fill_frame_parts(wm_theme_metrics *theme);
static void fill_title_parts(wm_theme_metrics *theme);
static void fill_buttons(wm_theme_metrics *theme);
static wm_decor_window default_window(void);

decor_fixture fixture_create(void)
{
    decor_fixture fixture = {0};

    fill_frame_parts(&fixture.theme);
    fill_title_parts(&fixture.theme);
    fill_buttons(&fixture.theme);
    fixture.config = wm_config_defaults().frame;
    fixture.window = default_window();
    return fixture;
}

void fixture_set_layout(decor_fixture *fixture, const char *button_layout)
{
    memset(fixture->config.button_layout, 0, sizeof fixture->config.button_layout);
    strncpy(fixture->config.button_layout, button_layout, WM_BUTTON_LAYOUT_MAX - 1);
}

void fixture_maximize(decor_fixture *fixture)
{
    fixture->window.maximized_horizontally = true;
    fixture->window.maximized_vertically = true;
}

void fixture_set_all_states(wm_size sizes[WM_FRAME_STATE_COUNT], wm_size size)
{
    sizes[WM_FRAME_ACTIVE] = size;
    sizes[WM_FRAME_INACTIVE] = size;
}

wm_decor_input fixture_input(const decor_fixture *fixture)
{
    return (wm_decor_input) {&fixture->theme, &fixture->config, &fixture->window};
}

wm_decor_layout fixture_layout(const decor_fixture *fixture)
{
    wm_decor_input input = fixture_input(fixture);
    wm_decor_layout layout;
    wm_decor_layout_compute(&input, &layout);
    return layout;
}

const wm_paint_op *fixture_find_op(const wm_decor_layout *layout, wm_pixmap_kind kind, int index)
{
    for (size_t i = 0; i < layout->op_count; i++) {
        if (layout->ops[i].pixmap.kind == kind && layout->ops[i].pixmap.index == index) {
            return &layout->ops[i];
        }
    }
    return NULL;
}

const wm_part_rect *fixture_find_part(const wm_decor_layout *layout, wm_part part)
{
    for (size_t i = 0; i < layout->part_count; i++) {
        if (layout->parts[i].part == part) {
            return &layout->parts[i];
        }
    }
    return NULL;
}

size_t fixture_count_ops(const wm_decor_layout *layout, wm_pixmap_kind kind)
{
    size_t count = 0;
    for (size_t i = 0; i < layout->op_count; i++) {
        count += layout->ops[i].pixmap.kind == kind;
    }
    return count;
}

wm_button_image fixture_button_image(const decor_fixture *fixture, wm_button button)
{
    wm_decor_layout layout = fixture_layout(fixture);
    const wm_paint_op *op = fixture_find_op(&layout, WM_PIXMAP_BUTTON, button);
    return op == NULL ? WM_IMAGE_COUNT : (wm_button_image) op->pixmap.image;
}

static void fill_frame_parts(wm_theme_metrics *theme)
{
    wm_size top_corner = {FIXTURE_TOP_CORNER_WIDTH, FIXTURE_TITLE_HEIGHT};
    wm_size bottom_corner = {FIXTURE_BOTTOM_CORNER, FIXTURE_BOTTOM_CORNER};

    fixture_set_all_states(theme->side[WM_SIDE_LEFT], (wm_size) {FIXTURE_SIDE, 1});
    fixture_set_all_states(theme->side[WM_SIDE_RIGHT], (wm_size) {FIXTURE_SIDE, 1});
    fixture_set_all_states(theme->side[WM_SIDE_BOTTOM], (wm_size) {1, FIXTURE_SIDE});
    fixture_set_all_states(theme->corner[WM_CORNER_TOP_LEFT], top_corner);
    fixture_set_all_states(theme->corner[WM_CORNER_TOP_RIGHT], top_corner);
    fixture_set_all_states(theme->corner[WM_CORNER_BOTTOM_LEFT], bottom_corner);
    fixture_set_all_states(theme->corner[WM_CORNER_BOTTOM_RIGHT], bottom_corner);
}

static void fill_title_parts(wm_theme_metrics *theme)
{
    for (int part = 0; part < WM_TITLE_PART_COUNT; part++) {
        wm_size size = {title_part_widths[part], FIXTURE_TITLE_HEIGHT};
        fixture_set_all_states(theme->title[part], size);
    }
}

static void fill_buttons(wm_theme_metrics *theme)
{
    wm_size size = {FIXTURE_BUTTON, FIXTURE_BUTTON};
    for (int button = 0; button < WM_BUTTON_COUNT; button++) {
        theme->button[button][WM_IMAGE_ACTIVE] = size;
        theme->button[button][WM_IMAGE_INACTIVE] = size;
    }
}

static wm_decor_window default_window(void)
{
    return (wm_decor_window) {
        .client_size = {FIXTURE_CLIENT_WIDTH, FIXTURE_CLIENT_HEIGHT},
        .title_text_size = {FIXTURE_TEXT_WIDTH, FIXTURE_TEXT_HEIGHT},
        .state = WM_FRAME_ACTIVE,
        .has_border = true,
        .has_menu = true,
        .has_stick = true,
        .can_hide = true,
        .can_maximize = true,
        .has_close = true,
    };
}
