#include "decor_private.h"

enum {
    MIN_APP_ICON_SIZE = 2,
};

static void add_button(const decor_plan *plan, wm_decor_layout *layout, wm_button button);
static void add_button_image(const decor_plan *plan, wm_decor_layout *layout, wm_button button);
static void add_menu_title_slice(const decor_plan *plan, wm_decor_layout *layout,
                                 wm_button_image image);
static void add_app_icon(const decor_plan *plan, wm_decor_layout *layout);
static wm_rect app_icon_rect(const decor_plan *plan, wm_size pixmap);
static bool shows_app_icon(const decor_plan *plan);
static bool uses_stretched_title(const decor_frame *frame);
static int title_slice_offset(const decor_frame *frame);
static wm_rect menu_pixmap_clip(const decor_plan *plan, wm_size pixmap);

void decor_paint_buttons(const decor_plan *plan, wm_decor_layout *layout)
{
    for (int button = 0; button < WM_BUTTON_COUNT; button++) {
        if (plan->buttons->visible[button]) {
            add_button(plan, layout, (wm_button) button);
        }
    }
    if (shows_app_icon(plan)) {
        add_app_icon(plan, layout);
    }
}

static void add_button(const decor_plan *plan, wm_decor_layout *layout, wm_button button)
{
    const decor_frame *frame = plan->frame;
    wm_button_image image = decor_button_image(frame, button);

    if (button == WM_BUTTON_MENU && shows_app_icon(plan) && uses_stretched_title(frame)) {
        add_menu_title_slice(plan, layout, image);
        return;
    }
    add_button_image(plan, layout, button);
}

static void add_button_image(const decor_plan *plan, wm_decor_layout *layout, wm_button button)
{
    const decor_frame *frame = plan->frame;
    wm_button_image image = decor_button_image(frame, button);
    wm_rect rect = plan->buttons->rect[button];
    wm_size size = frame->theme->button[button][image];
    wm_paint_op op = {
        .pixmap = {WM_PIXMAP_BUTTON, button, image},
        .dest = {rect.x, rect.y, size.width, size.height},
        .origin = {rect.x, rect.y},
        .mode = WM_FILL_PLAIN,
        .clip = rect,
    };
    decor_add_op(layout, frame->theme, op);
}

static void add_menu_title_slice(const decor_plan *plan, wm_decor_layout *layout,
                                 wm_button_image image)
{
    const decor_frame *frame = plan->frame;
    wm_rect rect = plan->buttons->rect[WM_BUTTON_MENU];
    wm_size size = frame->theme->button[WM_BUTTON_MENU][image];
    int x = rect.x - title_slice_offset(frame);
    wm_paint_op op = {
        .pixmap = {WM_PIXMAP_SIDE_STRETCH, WM_SIDE_TOP, frame->state},
        .dest = {x, rect.y, frame->top_width, size.height},
        .origin = {x, rect.y},
        .mode = WM_FILL_STRETCH_HORIZONTAL,
        .clip = menu_pixmap_clip(plan, size),
    };
    decor_add_op(layout, frame->theme, op);
}

static void add_app_icon(const decor_plan *plan, wm_decor_layout *layout)
{
    const decor_frame *frame = plan->frame;
    wm_button_image image = decor_button_image(frame, WM_BUTTON_MENU);
    wm_size pixmap = frame->theme->button[WM_BUTTON_MENU][image];
    wm_rect icon = app_icon_rect(plan, pixmap);
    wm_paint_op op = {
        .pixmap = {WM_PIXMAP_APP_ICON, 0, 0},
        .dest = icon,
        .origin = {icon.x, icon.y},
        .mode = WM_FILL_PLAIN,
        .clip = menu_pixmap_clip(plan, pixmap),
    };
    decor_add_op(layout, frame->theme, op);
}

static wm_rect app_icon_rect(const decor_plan *plan, wm_size pixmap)
{
    wm_size base = plan->frame->theme->button[WM_BUTTON_MENU][WM_IMAGE_ACTIVE];
    wm_rect rect = plan->buttons->rect[WM_BUTTON_MENU];
    int size = wm_min(base.width, base.height);
    int width = wm_min(size, pixmap.width);
    int height = wm_min(size, pixmap.height);

    if (size < MIN_APP_ICON_SIZE) {
        return (wm_rect) {rect.x, rect.y, 0, 0};
    }
    return (wm_rect) {
        rect.x + (pixmap.width - width + 1) / 2,
        rect.y + (pixmap.height - height + 1) / 2,
        width,
        height,
    };
}

static bool shows_app_icon(const decor_plan *plan)
{
    const decor_frame *frame = plan->frame;
    const wm_size *menu = frame->theme->button[WM_BUTTON_MENU];

    if (!plan->buttons->visible[WM_BUTTON_MENU] || !frame->window->has_icon) {
        return false;
    }
    return !wm_size_is_empty(menu[WM_IMAGE_ACTIVE])
           && !wm_size_is_empty(menu[decor_button_image(frame, WM_BUTTON_MENU)]);
}

static bool uses_stretched_title(const decor_frame *frame)
{
    return !wm_size_is_empty(frame->theme->side_stretch[WM_SIDE_TOP][frame->state]);
}

static int title_slice_offset(const decor_frame *frame)
{
    int x = decor_menu_button_x(frame);
    return x < 0 ? frame->top_width + x : x;
}

static wm_rect menu_pixmap_clip(const decor_plan *plan, wm_size pixmap)
{
    wm_rect rect = plan->buttons->rect[WM_BUTTON_MENU];
    wm_rect pixmap_rect = {rect.x, rect.y, pixmap.width, pixmap.height};
    return wm_rect_intersection(&rect, &pixmap_rect);
}
