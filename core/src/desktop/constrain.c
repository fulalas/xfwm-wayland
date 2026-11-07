#include "placement.h"

#include "desktop_state.h"
#include "frame.h"
#include "monitors.h"

enum {
    CLIENT_MIN_VISIBLE = 15,
};

typedef struct constrain_box {
    wm_window *window;
    wm_extents extents;
    wm_rect frame;
    wm_rect monitor;
    wm_rect work;
    int title_visible;
    int min_visible;
} constrain_box;

static constrain_box make_box(const wm_desktop *desktop, wm_window *window);
static void set_frame_x(constrain_box *box, int x);
static void set_frame_y(constrain_box *box, int y);
static void keep_on_monitor(constrain_box *box);
static void keep_off_struts(constrain_box *box);
static void keep_off_top_strut(constrain_box *box);
static bool intersects_strip(const constrain_box *box, wm_rect strip);
static bool title_overlaps(const constrain_box *box, wm_segment band);

void placement_constrain(wm_desktop *desktop, wm_window *window)
{
    if (window_is_fullscreen(window)) {
        return;
    }
    constrain_box box = make_box(desktop, window);
    keep_on_monitor(&box);
    keep_off_struts(&box);
    window_mark_dirty(window);
}

void placement_constrain_fully(wm_desktop *desktop, wm_window *window)
{
    if (window_is_fullscreen(window)) {
        return;
    }
    constrain_box box = make_box(desktop, window);
    if (wm_rect_right(&box.frame) >= wm_rect_right(&box.work)) {
        set_frame_x(&box, wm_rect_right(&box.work) - box.frame.width);
    }
    if (box.frame.x <= box.work.x) {
        set_frame_x(&box, box.work.x);
    }
    if (wm_rect_bottom(&box.frame) >= wm_rect_bottom(&box.work)) {
        set_frame_y(&box, wm_rect_bottom(&box.work) - box.frame.height);
    }
    if (box.frame.y <= box.work.y) {
        set_frame_y(&box, box.work.y);
    }
    window_mark_dirty(window);
}

static constrain_box make_box(const wm_desktop *desktop, wm_window *window)
{
    constrain_box box;
    box.window = window;
    box.extents = frame_extents(desktop, window);
    box.frame = frame_rect(desktop, window);
    wm_point center = wm_rect_center(&box.frame);
    box.monitor = monitors_full_at(desktop, center);
    box.work = monitors_work_area_at(desktop, center);
    box.title_visible = box.extents.top > 0 ? box.extents.top : frame_title_height(desktop);
    box.min_visible = box.title_visible > CLIENT_MIN_VISIBLE ? box.title_visible
                                                             : CLIENT_MIN_VISIBLE;
    return box;
}

static void set_frame_x(constrain_box *box, int x)
{
    box->frame.x = x;
    box->window->client.x = x + box->extents.left;
}

static void set_frame_y(constrain_box *box, int y)
{
    box->frame.y = y;
    box->window->client.y = y + box->extents.top;
}

static void keep_on_monitor(constrain_box *box)
{
    const wm_rect *monitor = &box->monitor;
    int visible = box->min_visible;
    if (wm_rect_right(&box->frame) <= monitor->x + visible) {
        set_frame_x(box, monitor->x + visible - box->frame.width);
    }
    if (box->frame.x + visible >= wm_rect_right(monitor)) {
        set_frame_x(box, wm_rect_right(monitor) - visible);
    }
    if (wm_rect_bottom(&box->frame) <= monitor->y + visible) {
        set_frame_y(box, monitor->y + visible - box->frame.height);
    }
    if (box->frame.y + visible >= wm_rect_bottom(monitor)) {
        set_frame_y(box, wm_rect_bottom(monitor) - visible);
    }
    if (box->frame.y <= monitor->y && box->frame.y >= monitor->y - box->extents.top) {
        set_frame_y(box, monitor->y);
    }
}

static void keep_off_struts(constrain_box *box)
{
    const wm_rect *work = &box->work;
    const wm_rect *monitor = &box->monitor;
    int visible = box->min_visible;
    wm_rect right = {wm_rect_right(work), monitor->y,
                     wm_rect_right(monitor) - wm_rect_right(work), monitor->height};
    if (intersects_strip(box, right) && box->frame.x >= wm_rect_right(work) - visible) {
        set_frame_x(box, wm_rect_right(work) - visible);
    }
    wm_rect left = {monitor->x, monitor->y, work->x - monitor->x, monitor->height};
    if (intersects_strip(box, left) && wm_rect_right(&box->frame) <= work->x + visible) {
        set_frame_x(box, work->x + visible - box->frame.width);
    }
    wm_rect bottom = {monitor->x, wm_rect_bottom(work), monitor->width,
                      wm_rect_bottom(monitor) - wm_rect_bottom(work)};
    if (intersects_strip(box, bottom) && box->frame.y >= wm_rect_bottom(work) - visible) {
        set_frame_y(box, wm_rect_bottom(work) - visible);
    }
    keep_off_top_strut(box);
}

static void keep_off_top_strut(constrain_box *box)
{
    const wm_rect *work = &box->work;
    const wm_rect *monitor = &box->monitor;
    wm_rect top = {monitor->x, monitor->y, monitor->width, work->y - monitor->y};
    if (!intersects_strip(box, top)) {
        return;
    }
    if (title_overlaps(box, (wm_segment) {monitor->y, work->y})) {
        set_frame_y(box, work->y);
    }
    if (wm_rect_bottom(&box->frame) <= work->y + box->min_visible) {
        set_frame_y(box, work->y + box->min_visible - box->frame.height);
    }
}

static bool intersects_strip(const constrain_box *box, wm_rect strip)
{
    return strip.width > 0 && strip.height > 0 && wm_rect_intersects(&strip, &box->frame);
}

static bool title_overlaps(const constrain_box *box, wm_segment band)
{
    wm_segment title = {box->frame.y, box->frame.y + box->title_visible};
    return wm_segments_overlap(title, band);
}
