#include "shadow.h"

#include <math.h>
#include <stdint.h>

#include <glib.h>

#include "wm/shadow.h"

enum {
    TEMPLATE_CORNERS = 3,
    ALPHA_MAX = 255,
    ALPHA_SHIFT = 24,
};

typedef struct gaussian_map {
    int size;
    double *data;
} gaussian_map;

static gaussian_map make_gaussian_map(double radius, int size);
static double gaussian(double radius, double x, double y);
static unsigned sum_gaussian(const gaussian_map *map, wm_point offset, int box);
static unsigned shadow_value(const gaussian_map *map, wm_point position, int template_size);
static int mirror(int position, int template_size);
static void fill_template(cairo_surface_t *surface, const gaussian_map *map);

shadow_template appearance_shadow_template_create(void)
{
    int kernel = wm_shadow_kernel_size();
    int size = TEMPLATE_CORNERS * kernel;
    gaussian_map map = make_gaussian_map(WM_SHADOW_RADIUS, kernel);
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);

    if (cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
        fill_template(surface, &map);
    }
    g_free(map.data);
    return (shadow_template) { surface, kernel };
}

static gaussian_map make_gaussian_map(double radius, int size)
{
    gaussian_map map = { size, g_new0(double, (gsize) size * (gsize) size) };
    int center = size / 2;
    double total = 0.0;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            double g = gaussian(radius, x - center, y - center);
            total += g;
            map.data[y * size + x] = g;
        }
    }
    for (int i = 0; i < size * size; i++) {
        map.data[i] /= total;
    }
    return map;
}

static double gaussian(double radius, double x, double y)
{
    return (1.0 / sqrt(2.0 * M_PI * radius)) * exp(-(x * x + y * y) / (2.0 * radius * radius));
}

static unsigned sum_gaussian(const gaussian_map *map, wm_point offset, int box)
{
    int center = map->size / 2;
    int x_start = MAX(center - offset.x, 0);
    int x_end = MIN(box + center - offset.x, map->size);
    int y_start = MAX(center - offset.y, 0);
    int y_end = MIN(box + center - offset.y, map->size);
    double value = 0.0;

    for (int fy = y_start; fy < y_end; fy++) {
        for (int fx = x_start; fx < x_end; fx++) {
            value += map->data[fy * map->size + fx];
        }
    }
    return (unsigned) (fmin(value, 1.0) * ALPHA_MAX);
}

static unsigned shadow_value(const gaussian_map *map, wm_point position, int template_size)
{
    int kernel = map->size;
    int center = kernel / 2;
    int column = MIN(mirror(position.x, template_size), kernel);
    int row = MIN(mirror(position.y, template_size), kernel);
    int short_side = MIN(column, row) - center;

    if (column < kernel && row < kernel) {
        wm_point corner = { MAX(column, row) - center, short_side };
        return sum_gaussian(map, corner, 2 * kernel);
    }
    return sum_gaussian(map, (wm_point) { short_side, center }, 2 * kernel);
}

static int mirror(int position, int template_size)
{
    return MIN(position, template_size - 1 - position);
}

static void fill_template(cairo_surface_t *surface, const gaussian_map *map)
{
    int size = cairo_image_surface_get_width(surface);
    int stride = cairo_image_surface_get_stride(surface);
    unsigned char *data = cairo_image_surface_get_data(surface);

    cairo_surface_flush(surface);
    for (int y = 0; y < size; y++) {
        uint32_t *row = (uint32_t *) (void *) (data + (size_t) y * (size_t) stride);
        for (int x = 0; x < size; x++) {
            unsigned alpha = shadow_value(map, (wm_point) { x, y }, size);
            row[x] = (uint32_t) alpha << ALPHA_SHIFT;
        }
    }
    cairo_surface_mark_dirty(surface);
}
