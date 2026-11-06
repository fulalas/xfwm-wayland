#ifndef COMPOSITOR_PAINTER_H
#define COMPOSITOR_PAINTER_H

#include <cairo.h>

#include "wm/decor.h"
#include "wm/geometry.h"
#include "wm/views.h"

typedef struct frame_paint_request {
    const wm_decor_layout *layout;
    wm_rect band;
    double scale;
    const char *title;
    cairo_surface_t *icon;
    wm_point origin;
} frame_paint_request;

typedef struct menu_layout {
    wm_size size;
    size_t count;
    wm_rect items[WM_MENU_ITEM_MAX];
} menu_layout;

typedef struct cycle_layout {
    wm_size size;
    size_t count;
    wm_rect items[WM_CYCLE_ITEM_MAX];
    wm_rect previews[WM_CYCLE_ITEM_MAX];
    wm_rect icons[WM_CYCLE_ITEM_MAX];
} cycle_layout;

typedef struct cycle_paint_request {
    const wm_cycle_view *view;
    const cycle_layout *layout;
    cairo_surface_t *const *icons;
    double scale;
} cycle_paint_request;

typedef struct menu_paint_request {
    const wm_menu_level *level;
    const menu_layout *layout;
    double scale;
} menu_paint_request;

typedef struct label_paint_request {
    const char *text;
    wm_size size;
    double scale;
} label_paint_request;

typedef struct shadow_template {
    cairo_surface_t *surface;
    int corner_size;
} shadow_template;

typedef struct compositor_osd_painter {
    void *self;
    menu_layout (*layout_menu)(void *self, const wm_menu_level *level);
    cairo_surface_t *(*paint_menu)(void *self, const menu_paint_request *request);
    cycle_layout (*layout_cycle)(void *self, const wm_cycle_view *view);
    cairo_surface_t *(*paint_cycle)(void *self, const cycle_paint_request *request);
    wm_size (*layout_label)(void *self, const char *text);
    cairo_surface_t *(*paint_label)(void *self, const label_paint_request *request);
} compositor_osd_painter;

typedef struct compositor_painter {
    void *self;
    cairo_surface_t *(*paint_frame)(void *self, const frame_paint_request *request);
    wm_size (*measure_title)(void *self, const char *title);
    shadow_template (*shadow_template)(void *self, double opacity);
    compositor_osd_painter osd;
} compositor_painter;

#endif
