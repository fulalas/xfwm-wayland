#include "canvas.h"

#include <math.h>
#include <stddef.h>

static const double pixel_epsilon = 1e-6;

static int pixels(int logical, double scale);
static int logical_length(int pixels, double scale);
static double fill_factor(int pixel_count, int logical);

canvas appearance_canvas_create(wm_size logical, double scale)
{
    double safe_scale = scale > 0.0 ? scale : 1.0;
    int width = pixels(logical.width, safe_scale);
    int height = pixels(logical.height, safe_scale);
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(surface);

    cairo_scale(cr, fill_factor(width, logical.width), fill_factor(height, logical.height));
    return (canvas) { surface, cr };
}

cairo_surface_t *appearance_canvas_finish(canvas *canvas)
{
    cairo_surface_t *surface = canvas->surface;

    cairo_destroy(canvas->cr);
    cairo_surface_flush(surface);
    *canvas = (struct canvas) { NULL, NULL };
    return surface;
}

void appearance_canvas_rounded_rect(cairo_t *cr, const wm_rect *rect, double radius)
{
    double left = rect->x;
    double top = rect->y;
    double right = rect->x + rect->width;
    double bottom = rect->y + rect->height;
    double r = fmin(radius, fmin(rect->width, rect->height) / 2.0);

    cairo_new_sub_path(cr);
    cairo_arc(cr, right - r, top + r, r, -M_PI / 2.0, 0.0);
    cairo_arc(cr, right - r, bottom - r, r, 0.0, M_PI / 2.0);
    cairo_arc(cr, left + r, bottom - r, r, M_PI / 2.0, M_PI);
    cairo_arc(cr, left + r, top + r, r, M_PI, 3.0 * M_PI / 2.0);
    cairo_close_path(cr);
}

void appearance_canvas_paint_fitted(cairo_t *cr, const canvas_image *image)
{
    const wm_rect *rect = &image->rect;
    wm_size size = image->surface != NULL ? appearance_surface_size(image->surface)
                                          : (wm_size) { 0, 0 };

    if (wm_size_is_empty(size)) {
        return;
    }
    double zoom = fmin((double) rect->width / size.width, (double) rect->height / size.height);
    double left = rect->x + (rect->width - size.width * zoom) / 2.0;
    double top = rect->y + (rect->height - size.height * zoom) / 2.0;
    double right = left + size.width * zoom;
    double bottom = top + size.height * zoom;
    cairo_user_to_device(cr, &left, &top);
    cairo_user_to_device(cr, &right, &bottom);
    left = round(left);
    top = round(top);
    cairo_save(cr);
    cairo_identity_matrix(cr);
    cairo_translate(cr, left, top);
    cairo_scale(cr, fmax(round(right) - left, 1.0) / size.width,
                fmax(round(bottom) - top, 1.0) / size.height);
    cairo_set_source_surface(cr, image->surface, 0, 0);
    cairo_paint_with_alpha(cr, image->alpha);
    cairo_restore(cr);
}

wm_size appearance_surface_size(cairo_surface_t *surface)
{
    return (wm_size) { cairo_image_surface_get_width(surface),
                       cairo_image_surface_get_height(surface) };
}

wm_size appearance_surface_logical_size(cairo_surface_t *surface, double scale)
{
    wm_size pixels = appearance_surface_size(surface);
    return (wm_size) { logical_length(pixels.width, scale), logical_length(pixels.height, scale) };
}

static int logical_length(int pixels, double scale)
{
    if (pixels <= 0) {
        return 0;
    }
    long length = lround(pixels / scale);
    return length > 0 ? (int) length : 1;
}

static int pixels(int logical, double scale)
{
    if (logical <= 0) {
        return 0;
    }
    return (int) ceil(logical * scale - pixel_epsilon);
}

static double fill_factor(int pixel_count, int logical)
{
    if (logical <= 0) {
        return 1.0;
    }
    return (double) pixel_count / logical;
}
