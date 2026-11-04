#include "find_cursor_paint.h"

#include <math.h>
#include <stddef.h>

static const double RED = 1.0;
static const double GREEN = 0.0;
static const double BLUE = 0.0;
static const double FILL_ALPHA = 0.5;
static const double STROKE_ALPHA = 1.0;

static void draw_rings(cairo_t *cr, const find_cursor_frame *frame);

cairo_surface_t *find_cursor_paint(const find_cursor_frame *frame, double scale)
{
    int half = find_cursor_half_size(frame);
    int pixels = (int) ceil(2 * half * scale);
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixels, pixels);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        return NULL;
    }
    cairo_t *cr = cairo_create(surface);
    cairo_scale(cr, pixels / (2.0 * half), pixels / (2.0 * half));
    cairo_translate(cr, half, half);
    draw_rings(cr, frame);
    cairo_destroy(cr);
    return surface;
}

static void draw_rings(cairo_t *cr, const find_cursor_frame *frame)
{
    cairo_arc(cr, 0, 0, frame->radius, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, RED, GREEN, BLUE, FILL_ALPHA);
    cairo_fill(cr);
    cairo_set_line_width(cr, FIND_CURSOR_LINE_WIDTH);
    cairo_set_source_rgba(cr, RED, GREEN, BLUE, STROKE_ALPHA);
    for (int ring = 0; ring < frame->rings; ring++) {
        cairo_new_sub_path(cr);
        cairo_arc(cr, 0, 0, find_cursor_ring_radius(frame, ring), 0, 2 * M_PI);
    }
    cairo_stroke(cr);
}
