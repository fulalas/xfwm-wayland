#include "tile.h"

#include "desktop_state.h"
#include "frame.h"
#include "maximize.h"
#include "monitors.h"
#include "size_hints.h"

enum {
    SPAN_FULL,
    SPAN_FIRST,
    SPAN_SECOND,
};

typedef struct tile_request {
    wm_point point;
    tile_mode mode;
    bool restore_position;
} tile_request;

typedef struct tile_fit {
    bool fits;
    wm_rect client;
} tile_fit;

static bool tile(wm_desktop *desktop, wm_window *window, const tile_request *request);
static tile_fit tile_rect(const wm_desktop *desktop, const wm_window *window,
                          const tile_request *request);
static wm_rect tile_frame(const wm_rect *full, tile_mode mode);
static wm_point span(int start, int length, int part);
static bool tile_window(wm_desktop *desktop, wm_window *window, tile_mode mode);

bool tile_window_at(wm_desktop *desktop, wm_window *window, const tile_placement *placement)
{
    tile_request request = {placement->point, placement->mode, false};
    return tile(desktop, window, &request);
}

void tile_toggle(wm_desktop *desktop, wm_window *window, tile_mode mode)
{
    if (window->tile != mode) {
        tile_window(desktop, window, mode);
        return;
    }
    window->tile = TILE_NONE;
    maximize_restore_geometry(window);
    window_placed(window);
    window_mark_dirty(window);
}

void tile_update_size(wm_desktop *desktop, wm_window *window)
{
    if (window->tile == TILE_NONE) {
        return;
    }
    tile_request request = {frame_center(desktop, window), window->tile, false};
    tile_fit fit = tile_rect(desktop, window, &request);
    if (fit.fits) {
        window->client = fit.client;
        window_mark_dirty(window);
    }
}

static bool tile(wm_desktop *desktop, wm_window *window, const tile_request *request)
{
    if (!window_can_tile(window)) {
        return false;
    }
    if (request->restore_position) {
        maximize_save_geometry(window);
    }
    uint32_t old_states = window->states;
    window->states &= ~(uint32_t) WM_STATE_MAXIMIZED;
    tile_fit fit = tile_rect(desktop, window, request);
    if (!fit.fits) {
        window->states = old_states;
        return false;
    }
    window->restore_size_pos = true;
    window->tile = request->mode;
    window->client = fit.client;
    window_placed(window);
    window_mark_dirty(window);
    return true;
}

static tile_fit tile_rect(const wm_desktop *desktop, const wm_window *window,
                          const tile_request *request)
{
    tile_fit fit = {false, {0, 0, 0, 0}};
    wm_rect full = monitors_work_area_at(desktop, request->point);
    wm_rect frame = tile_frame(&full, request->mode);
    if (frame.width == 0 && frame.height == 0) {
        return fit;
    }
    wm_extents extents = frame_extents(desktop, window);
    fit.client = wm_rect_shrink(&frame, &extents);
    fit.fits = size_hints_allow(window, (wm_size) {fit.client.width, fit.client.height});
    return fit;
}

static wm_rect tile_frame(const wm_rect *full, tile_mode mode)
{
    static const struct {
        tile_mode mode;
        int column;
        int row;
    } cells[] = {
        {TILE_UP, SPAN_FULL, SPAN_FIRST}, {TILE_DOWN, SPAN_FULL, SPAN_SECOND},
        {TILE_LEFT, SPAN_FIRST, SPAN_FULL}, {TILE_RIGHT, SPAN_SECOND, SPAN_FULL},
        {TILE_DOWN_LEFT, SPAN_FIRST, SPAN_SECOND}, {TILE_DOWN_RIGHT, SPAN_SECOND, SPAN_SECOND},
        {TILE_UP_LEFT, SPAN_FIRST, SPAN_FIRST}, {TILE_UP_RIGHT, SPAN_SECOND, SPAN_FIRST},
    };
    for (size_t i = 0; i < sizeof(cells) / sizeof(cells[0]); i++) {
        if (cells[i].mode == mode) {
            wm_point x = span(full->x, full->width, cells[i].column);
            wm_point y = span(full->y, full->height, cells[i].row);
            return (wm_rect) {x.x, y.x, x.y, y.y};
        }
    }
    return (wm_rect) {0, 0, 0, 0};
}

static wm_point span(int start, int length, int part)
{
    int half = length / 2;
    if (part == SPAN_FIRST) {
        return (wm_point) {start, half};
    }
    if (part == SPAN_SECOND) {
        return (wm_point) {start + half, length - half};
    }
    return (wm_point) {start, length};
}

static bool tile_window(wm_desktop *desktop, wm_window *window, tile_mode mode)
{
    tile_request request = {frame_center(desktop, window), mode, true};
    return tile(desktop, window, &request);
}
