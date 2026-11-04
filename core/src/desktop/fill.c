#include "fill.h"

#include "desktop_state.h"
#include "frame.h"
#include "monitors.h"

typedef struct neighbours {
    bool has_west;
    bool has_east;
    bool has_north;
    bool has_south;
    int west;
    int east;
    int north;
    int south;
} neighbours;

static neighbours find_neighbours(const wm_desktop *desktop, const wm_window *window,
                                  unsigned mode);
static void consider_horizontal(neighbours *found, const wm_rect *frame, const wm_rect *other);
static void consider_vertical(neighbours *found, const wm_rect *frame, const wm_rect *other);
static wm_rect fill_area(const wm_desktop *desktop, const wm_window *window, unsigned mode);
static wm_rect filled_client(const wm_extents *extents, const wm_rect *full,
                             const neighbours *found);

void fill_window(wm_desktop *desktop, wm_window *window, unsigned mode)
{
    if (!window_can_fill(window)) {
        return;
    }
    neighbours found = find_neighbours(desktop, window, mode);
    wm_rect full = fill_area(desktop, window, mode);
    wm_extents extents = frame_extents(desktop, window);
    wm_rect target = filled_client(&extents, &full, &found);
    if (mode & FILL_HORIZONTAL) {
        window->client.x = target.x;
        window->client.width = target.width;
    }
    if (mode & FILL_VERTICAL) {
        window->client.y = target.y;
        window->client.height = target.height;
    }
    window_mark_dirty(window);
}

static neighbours find_neighbours(const wm_desktop *desktop, const wm_window *window,
                                  unsigned mode)
{
    neighbours found = {0};
    wm_rect frame = frame_rect(desktop, window);
    for (size_t i = 0; i < desktop->stack.count; i++) {
        const wm_window *other = desktop->stack.items[i];
        if (other == window || !other->visible || other->layer != window->layer) {
            continue;
        }
        wm_rect other_frame = frame_rect(desktop, other);
        if (mode & FILL_HORIZONTAL) {
            consider_horizontal(&found, &frame, &other_frame);
        }
        if (mode & FILL_VERTICAL) {
            consider_vertical(&found, &frame, &other_frame);
        }
    }
    return found;
}

static void consider_horizontal(neighbours *found, const wm_rect *frame, const wm_rect *other)
{
    if (!wm_segments_overlap(frame->y, wm_rect_bottom(frame), other->y, wm_rect_bottom(other))) {
        return;
    }
    int other_right = wm_rect_right(other);
    if (other_right <= frame->x && (!found->has_west || found->west < other_right)) {
        found->has_west = true;
        found->west = other_right;
    }
    if (wm_rect_right(frame) <= other->x && (!found->has_east || other->x < found->east)) {
        found->has_east = true;
        found->east = other->x;
    }
}

static void consider_vertical(neighbours *found, const wm_rect *frame, const wm_rect *other)
{
    if (!wm_segments_overlap(frame->x, wm_rect_right(frame), other->x, wm_rect_right(other))) {
        return;
    }
    if (wm_rect_bottom(other) <= frame->y
        && (!found->has_north || found->north < wm_rect_bottom(other))) {
        found->has_north = true;
        found->north = wm_rect_bottom(other);
    }
    if (wm_rect_bottom(frame) <= other->y && (!found->has_south || other->y < found->south)) {
        found->has_south = true;
        found->south = other->y;
    }
}

static wm_rect fill_area(const wm_desktop *desktop, const wm_window *window, unsigned mode)
{
    wm_rect frame = frame_rect(desktop, window);
    wm_rect full = monitors_work_area_at(desktop, wm_rect_center(&frame));
    if (!(mode & FILL_HORIZONTAL)) {
        full.x = frame.x;
        full.width = frame.width;
    }
    if (!(mode & FILL_VERTICAL)) {
        full.y = frame.y;
        full.height = frame.height;
    }
    return full;
}

static wm_rect filled_client(const wm_extents *extents, const wm_rect *full,
                             const neighbours *found)
{
    wm_rect client;
    client.x = full->x + extents->left;
    if (found->has_west) {
        client.x += wm_max(found->west - full->x, 0);
    }
    client.width = full->width - extents->right - (client.x - full->x);
    if (found->has_east) {
        client.width -= wm_max(full->width - (found->east - full->x), 0);
    }
    client.y = full->y + extents->top;
    if (found->has_north) {
        client.y += wm_max(found->north - full->y, 0);
    }
    client.height = full->height - extents->bottom - (client.y - full->y);
    if (found->has_south) {
        client.height -= wm_max(full->height - (found->south - full->y), 0);
    }
    return client;
}
