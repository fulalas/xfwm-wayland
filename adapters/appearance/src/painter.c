#include "appearance_internal.h"
#include "label_painter.h"
#include "menu_layout.h"
#include "menu_painter.h"
#include "shadow.h"
#include "tabwin_layout.h"
#include "tabwin_painter.h"

static cairo_surface_t *paint_frame(void *self, const frame_paint_request *request);
static wm_size measure_title(void *self, const char *title);
static shadow_template make_shadow_template(void *self, double opacity);
static menu_layout layout_menu(void *self, const wm_menu_level *level);
static cairo_surface_t *paint_menu(void *self, const menu_paint_request *request);
static cycle_layout layout_cycle(void *self, const wm_cycle_view *view);
static cairo_surface_t *paint_cycle(void *self, const cycle_paint_request *request);
static wm_size layout_label(void *self, const char *text);
static cairo_surface_t *paint_label(void *self, const label_paint_request *request);

compositor_painter appearance_painter(appearance *appearance)
{
    return (compositor_painter) {
        .self = appearance,
        .paint_frame = paint_frame,
        .measure_title = measure_title,
        .shadow_template = make_shadow_template,
        .osd = {
            .self = appearance,
            .layout_menu = layout_menu,
            .paint_menu = paint_menu,
            .layout_cycle = layout_cycle,
            .paint_cycle = paint_cycle,
            .layout_label = layout_label,
            .paint_label = paint_label,
        },
    };
}

static cairo_surface_t *paint_frame(void *self, const frame_paint_request *request)
{
    frame_painter painter = appearance_frame_painter(self);

    return appearance_frame_paint(&painter, request);
}

static wm_size measure_title(void *self, const char *title)
{
    return appearance_title_measure(appearance_title_style(self), title);
}

static shadow_template make_shadow_template(void *self, double opacity)
{
    (void) self;
    return appearance_shadow_template_create(opacity);
}

static menu_layout layout_menu(void *self, const wm_menu_level *level)
{
    ui_text ui = appearance_ui_text(self);

    return appearance_menu_layout(ui.font, level);
}

static cairo_surface_t *paint_menu(void *self, const menu_paint_request *request)
{
    ui_text ui = appearance_ui_text(self);

    return appearance_menu_paint(&ui, request);
}

static cycle_layout layout_cycle(void *self, const wm_cycle_view *view)
{
    ui_text ui = appearance_ui_text(self);

    return appearance_tabwin_layout(view, appearance_text_line_height(ui.font));
}

static cairo_surface_t *paint_cycle(void *self, const cycle_paint_request *request)
{
    ui_text ui = appearance_ui_text(self);

    return appearance_tabwin_paint(&ui, request);
}

static wm_size layout_label(void *self, const char *text)
{
    ui_text ui = appearance_ui_text(self);

    return appearance_label_layout(ui.font, text);
}

static cairo_surface_t *paint_label(void *self, const label_paint_request *request)
{
    ui_text ui = appearance_ui_text(self);

    return appearance_label_paint(&ui, request);
}
