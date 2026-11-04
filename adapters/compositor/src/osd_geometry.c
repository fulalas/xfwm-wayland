#include "osd_geometry.h"

typedef struct span {
    int start;
    int length;
} span;

static int clamp_span(span item, span area);

wm_point osd_clamp_origin(wm_point anchor, wm_size size, const wm_rect *area)
{
    wm_point origin = {
        clamp_span((span){anchor.x, size.width}, (span){area->x, area->width}),
        clamp_span((span){anchor.y, size.height}, (span){area->y, area->height}),
    };
    return origin;
}

wm_rect osd_item_row(const wm_rect *panel, const wm_rect *item)
{
    wm_rect row = {panel->x, panel->y + item->y, panel->width, item->height};
    return row;
}

wm_point osd_submenu_origin(const wm_rect *row, wm_size size, const wm_rect *area)
{
    wm_point origin = {row->x + row->width, row->y};
    if (origin.x + size.width > area->x + area->width) {
        origin.x = row->x - size.width;
    }
    return osd_clamp_origin(origin, size, area);
}

wm_point osd_center_origin(wm_size size, const wm_rect *area)
{
    wm_point origin = {
        area->x + (area->width - size.width) / 2,
        area->y + (area->height - size.height) / 2,
    };
    return origin;
}

wm_rect osd_fit_preview(wm_size content, const wm_rect *slot)
{
    if (content.width <= 0 || content.height <= 0 || slot->width <= 0 || slot->height <= 0) {
        return (wm_rect){slot->x, slot->y, 0, 0};
    }
    double scale_x = (double) slot->width / content.width;
    double scale_y = (double) slot->height / content.height;
    double scale = scale_x < scale_y ? scale_x : scale_y;
    if (scale > 1.0) {
        scale = 1.0;
    }
    int width = (int) (content.width * scale);
    int height = (int) (content.height * scale);
    wm_rect fitted = {
        slot->x + (slot->width - width) / 2,
        slot->y + (slot->height - height) / 2,
        width,
        height,
    };
    return fitted;
}

int osd_item_at(const wm_rect *items, size_t count, wm_point point)
{
    for (size_t i = 0; i < count; i++) {
        if (wm_rect_contains(&items[i], point)) {
            return (int) i;
        }
    }
    return -1;
}

static int clamp_span(span item, span area)
{
    int last = area.start + area.length - item.length;
    int start = item.start > last ? last : item.start;
    return start < area.start ? area.start : start;
}
