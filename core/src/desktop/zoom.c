#include "zoom.h"

#include "desktop_state.h"

enum {
    ZOOM_UNIT = 1 << 16,
    ZOOM_STEP = 4096,
    ZOOM_MIN_INVERSE = 1 << 10,
};

static bool is_zoomed(const zoom_state *zoom);
static void recenter(wm_desktop *desktop, wm_point pointer);

void zoom_init(wm_desktop *desktop)
{
    desktop->zoom.inverse_scale = ZOOM_UNIT;
}

void zoom_reset(wm_desktop *desktop)
{
    zoom_state *zoom = &desktop->zoom;
    if (is_zoomed(zoom)) {
        zoom->dirty = true;
    }
    zoom->inverse_scale = ZOOM_UNIT;
}

void zoom_in(wm_desktop *desktop, wm_point pointer)
{
    zoom_state *zoom = &desktop->zoom;
    zoom->inverse_scale -= ZOOM_STEP;
    if (zoom->inverse_scale < ZOOM_MIN_INVERSE) {
        zoom->inverse_scale = ZOOM_MIN_INVERSE;
    }
    recenter(desktop, pointer);
}

void zoom_out(wm_desktop *desktop, wm_point pointer)
{
    zoom_state *zoom = &desktop->zoom;
    if (!is_zoomed(zoom)) {
        return;
    }
    zoom->inverse_scale += ZOOM_STEP;
    if (zoom->inverse_scale >= ZOOM_UNIT) {
        zoom->inverse_scale = ZOOM_UNIT;
    }
    recenter(desktop, pointer);
}

void zoom_follow_pointer(wm_desktop *desktop, wm_point pointer)
{
    zoom_state *zoom = &desktop->zoom;
    if (is_zoomed(zoom) && (zoom->center.x != pointer.x || zoom->center.y != pointer.y)) {
        recenter(desktop, pointer);
    }
}

void zoom_present(wm_desktop *desktop)
{
    zoom_state *zoom = &desktop->zoom;
    if (!zoom->dirty) {
        return;
    }
    zoom->dirty = false;
    wm_zoom_view view;
    view.level = (double) ZOOM_UNIT / (double) zoom->inverse_scale;
    view.center = zoom->center;
    view.zoom_pointer = desktop->config.mouse.zoom_pointer;
    desktop->ports.presenter.zoom_changed(desktop->ports.presenter.self, &view);
}

static bool is_zoomed(const zoom_state *zoom)
{
    return zoom->inverse_scale < ZOOM_UNIT;
}

static void recenter(wm_desktop *desktop, wm_point pointer)
{
    desktop->zoom.center = pointer;
    desktop->zoom.dirty = true;
}
