#include "x11_gravity.h"

#include <xcb/xproto.h>

typedef enum axis_anchor {
    ANCHOR_STATIC,
    ANCHOR_START,
    ANCHOR_CENTER,
    ANCHOR_END,
} axis_anchor;

typedef struct anchors {
    axis_anchor horizontal;
    axis_anchor vertical;
} anchors;

static const anchors gravity_anchors[] = {
    [XCB_GRAVITY_NORTH_WEST] = {ANCHOR_START, ANCHOR_START},
    [XCB_GRAVITY_NORTH] = {ANCHOR_CENTER, ANCHOR_START},
    [XCB_GRAVITY_NORTH_EAST] = {ANCHOR_END, ANCHOR_START},
    [XCB_GRAVITY_WEST] = {ANCHOR_START, ANCHOR_CENTER},
    [XCB_GRAVITY_CENTER] = {ANCHOR_CENTER, ANCHOR_CENTER},
    [XCB_GRAVITY_EAST] = {ANCHOR_END, ANCHOR_CENTER},
    [XCB_GRAVITY_SOUTH_WEST] = {ANCHOR_START, ANCHOR_END},
    [XCB_GRAVITY_SOUTH] = {ANCHOR_CENTER, ANCHOR_END},
    [XCB_GRAVITY_SOUTH_EAST] = {ANCHOR_END, ANCHOR_END},
};

static anchors anchors_for(int gravity);
static int frame_offset(axis_anchor anchor, int before, int after);
static int kept_offset(axis_anchor anchor, int old_size, int new_size);

wm_rect x11_gravity_configure(const x11_framed_window *window, const x11_configure *request)
{
    const wm_rect *client = &window->client;
    uint32_t fields = request->fields;
    wm_rect result = *client;
    result.width = fields & XCB_CONFIG_WINDOW_WIDTH ? request->values.width : client->width;
    result.height = fields & XCB_CONFIG_WINDOW_HEIGHT ? request->values.height : client->height;
    anchors a = anchors_for(request->gravity);
    wm_point reference = {request->values.x, request->values.y};
    wm_point placed = x11_gravity_place(reference, request->gravity, &window->extents);
    result.x = fields & XCB_CONFIG_WINDOW_X
                   ? placed.x
                   : client->x + kept_offset(a.horizontal, client->width, result.width);
    result.y = fields & XCB_CONFIG_WINDOW_Y
                   ? placed.y
                   : client->y + kept_offset(a.vertical, client->height, result.height);
    return result;
}

wm_point x11_gravity_place(wm_point reference, int gravity, const wm_extents *extents)
{
    anchors a = anchors_for(gravity);
    return (wm_point){
        reference.x + frame_offset(a.horizontal, extents->left, extents->right),
        reference.y + frame_offset(a.vertical, extents->top, extents->bottom),
    };
}

static anchors anchors_for(int gravity)
{
    if (gravity < XCB_GRAVITY_NORTH_WEST || gravity > XCB_GRAVITY_SOUTH_EAST) {
        return (anchors){ANCHOR_STATIC, ANCHOR_STATIC};
    }
    return gravity_anchors[gravity];
}

static int frame_offset(axis_anchor anchor, int before, int after)
{
    switch (anchor) {
    case ANCHOR_START:
        return before;
    case ANCHOR_CENTER:
        return (before - after + 1) / 2;
    case ANCHOR_END:
        return -after;
    default:
        return 0;
    }
}

static int kept_offset(axis_anchor anchor, int old_size, int new_size)
{
    switch (anchor) {
    case ANCHOR_CENTER:
        return (old_size - new_size) / 2;
    case ANCHOR_END:
        return old_size - new_size;
    default:
        return 0;
    }
}
