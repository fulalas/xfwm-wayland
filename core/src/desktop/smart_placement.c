#include "placement.h"

#include <limits.h>

#include "desktop_state.h"
#include "frame.h"

typedef struct smart_range {
    int min;
    int max;
    int offset;
} smart_range;

typedef struct smart_search {
    const wm_desktop *desktop;
    const wm_window *window;
    wm_rect area;
    wm_extents extents;
    wm_rect frame;
    smart_range x;
    smart_range y;
} smart_search;

typedef struct smart_steps {
    int next_x;
    int next_y;
    bool want_y;
} smart_steps;

typedef struct smart_best {
    long overlaps;
    wm_point position;
} smart_best;

static smart_search make_search(const wm_desktop *desktop, const wm_window *window,
                                const wm_rect *area);
static int search_row(const smart_search *search, int test_y, smart_best *best);
static long scan(const smart_search *search, wm_point test, smart_steps *steps);
static bool is_obstacle(const smart_search *search, const wm_window *other, wm_rect *frame);
static int next_edge(const smart_range *range, int test, wm_segment other);
static int closer_step(int current, int candidate, int test);
static int advance(const smart_range *range, int next, int current);

void placement_smart(const wm_desktop *desktop, wm_window *window, const wm_rect *area)
{
    smart_search search = make_search(desktop, window, area);
    smart_best best = {LONG_MAX, {search.x.min, search.y.min}};
    int test_y = search.y.min;
    do {
        int next_y = search_row(&search, test_y, &best);
        if (best.overlaps == 0) {
            break;
        }
        test_y = advance(&search.y, next_y, test_y);
    } while (test_y <= search.y.max);
    window->client.x = best.position.x;
    window->client.y = best.position.y;
}

static smart_search make_search(const wm_desktop *desktop, const wm_window *window,
                                const wm_rect *area)
{
    smart_search search;
    search.desktop = desktop;
    search.window = window;
    search.area = *area;
    search.extents = frame_extents(desktop, window);
    search.frame = frame_rect(desktop, window);
    search.x = (smart_range) {area->x + search.extents.left,
                              wm_rect_right(area) - window->client.width - search.extents.right,
                              search.extents.left};
    search.y = (smart_range) {area->y + search.extents.top,
                              wm_rect_bottom(area) - window->client.height - search.extents.bottom,
                              search.extents.top};
    return search;
}

static int search_row(const smart_search *search, int test_y, smart_best *best)
{
    smart_steps steps = {INT_MAX, INT_MAX, true};
    int test_x = search->x.min;
    do {
        steps.next_x = INT_MAX;
        long overlaps = scan(search, (wm_point) {test_x, test_y}, &steps);
        steps.want_y = false;
        if (overlaps < best->overlaps) {
            best->overlaps = overlaps;
            best->position = (wm_point) {test_x, test_y};
            if (overlaps == 0) {
                return steps.next_y;
            }
        }
        test_x = advance(&search->x, steps.next_x, test_x);
    } while (test_x <= search->x.max);
    return steps.next_y;
}

static long scan(const smart_search *search, wm_point test, smart_steps *steps)
{
    long overlaps = 0;
    wm_rect candidate = {test.x - search->extents.left, test.y - search->extents.top,
                         search->frame.width, search->frame.height};
    const window_list *stack = &search->desktop->stack;
    for (size_t i = 0; i < stack->count; i++) {
        wm_rect other;
        if (!is_obstacle(search, stack->items[i], &other)) {
            continue;
        }
        overlaps += wm_rect_overlap_area(&candidate, &other);
        int next_x = next_edge(&search->x, test.x, wm_rect_columns(&other));
        steps->next_x = closer_step(steps->next_x, next_x, test.x);
        if (steps->want_y) {
            int next_y = next_edge(&search->y, test.y, wm_rect_rows(&other));
            steps->next_y = closer_step(steps->next_y, next_y, test.y);
        }
    }
    return overlaps;
}

static bool is_obstacle(const smart_search *search, const wm_window *other, wm_rect *frame)
{
    if (other == search->window || other->type == WM_TYPE_DESKTOP || !other->visible
        || other->workspace != search->window->workspace) {
        return false;
    }
    *frame = frame_rect(search->desktop, other);
    const wm_rect *area = &search->area;
    if (frame->x >= wm_rect_right(area) || wm_rect_right(frame) < area->x) {
        return false;
    }
    return frame->y < wm_rect_bottom(area) && wm_rect_bottom(frame) >= area->y;
}

static int next_edge(const smart_range *range, int test, wm_segment other)
{
    int edge = test > other.start ? other.end : other.start;
    return wm_min(edge, range->max);
}

static int closer_step(int current, int candidate, int test)
{
    return candidate < current && candidate > test ? candidate : current;
}

static int advance(const smart_range *range, int next, int current)
{
    if (next == INT_MAX) {
        return current + 1;
    }
    return wm_min(wm_max(next + range->offset, next), range->max);
}
