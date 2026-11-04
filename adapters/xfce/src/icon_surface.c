#include "icon_surface.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdint.h>

enum {
    CHANNEL_MAX = 255,
    RGB_CHANNELS = 3,
    ALPHA_SHIFT = 24,
    RED_SHIFT = 16,
    GREEN_SHIFT = 8,
};

static cairo_surface_t *surface_from_pixbuf(GdkPixbuf *pixbuf);
static void fill_surface(cairo_surface_t *surface, GdkPixbuf *pixbuf);
static uint32_t premultiply(const guchar *pixel, int channels);

cairo_surface_t *icon_surface_load(const char *path, int size)
{
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(path, size, size, TRUE, &error);
    cairo_surface_t *surface;

    if (!pixbuf) {
        g_warning("Cannot load the icon %s: %s", path, error->message);
        g_error_free(error);
        return NULL;
    }
    surface = surface_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    return surface;
}

static cairo_surface_t *surface_from_pixbuf(GdkPixbuf *pixbuf)
{
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        return NULL;
    }
    fill_surface(surface, pixbuf);
    return surface;
}

static void fill_surface(cairo_surface_t *surface, GdkPixbuf *pixbuf)
{
    unsigned char *target = cairo_image_surface_get_data(surface);
    int target_stride = cairo_image_surface_get_stride(surface);
    const guchar *source = gdk_pixbuf_read_pixels(pixbuf);
    int source_stride = gdk_pixbuf_get_rowstride(pixbuf);
    int channels = gdk_pixbuf_get_n_channels(pixbuf);

    cairo_surface_flush(surface);
    for (int y = 0; y < gdk_pixbuf_get_height(pixbuf); y++) {
        const guchar *source_row = source + y * source_stride;
        uint32_t *target_row = (uint32_t *) (void *) (target + y * target_stride);
        for (int x = 0; x < gdk_pixbuf_get_width(pixbuf); x++) {
            target_row[x] = premultiply(source_row + x * channels, channels);
        }
    }
    cairo_surface_mark_dirty(surface);
}

static uint32_t premultiply(const guchar *pixel, int channels)
{
    uint32_t alpha = channels > RGB_CHANNELS ? pixel[RGB_CHANNELS] : CHANNEL_MAX;
    uint32_t red = (pixel[0] * alpha + CHANNEL_MAX / 2) / CHANNEL_MAX;
    uint32_t green = (pixel[1] * alpha + CHANNEL_MAX / 2) / CHANNEL_MAX;
    uint32_t blue = (pixel[2] * alpha + CHANNEL_MAX / 2) / CHANNEL_MAX;

    return alpha << ALPHA_SHIFT | red << RED_SHIFT | green << GREEN_SHIFT | blue;
}
