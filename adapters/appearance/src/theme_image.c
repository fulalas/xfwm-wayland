#include "theme_image.h"

#include <stdint.h>
#include <string.h>

#include <glib.h>

#include "mysvg.h"

static const char *const overlay_extensions[] = { "svg", "png", "gif", "jpg", "bmp" };
static const char svg_extension[] = "svg";

enum {
    OPAQUE = 0xFF,
    RED_SHIFT = 16,
    GREEN_SHIFT = 8,
    ALPHA_SHIFT = 24,
    RGBA_CHANNELS = 4,
    RED_CHANNEL = 0,
    GREEN_CHANNEL = 1,
    BLUE_CHANNEL = 2,
    ALPHA_CHANNEL = 3,
};

static cairo_surface_t *load_bitmap(const theme_image_source *source, const char *name);
static cairo_surface_t *load_svg(const theme_image_source *source, const char *name);
static xfwmColorSymbol *svg_colors(const xpm_symbols *symbols);
static cairo_surface_t *surface_from_pixbuf(const GdkPixbuf *pixbuf);
static GdkPixbuf *load_xpm(const theme_image_source *source, const char *name);
static GdkPixbuf *load_overlay(const char *dir, const char *name);
static GdkPixbuf *compose(GdkPixbuf *base, GdkPixbuf *overlay);
static void copy_row(const GdkPixbuf *pixbuf, int y, uint32_t *dest);
static uint32_t cut_out_pixel(const guchar *pixel, int channels);

bool appearance_theme_images_scalable(const char *dir)
{
    GDir *entries = g_dir_open(dir, 0, NULL);
    bool has_svg = false;

    for (const char *file; !has_svg && entries != NULL && (file = g_dir_read_name(entries));) {
        const char *dot = strrchr(file, '.');
        has_svg = dot != NULL && strcmp(dot + 1, svg_extension) == 0;
    }
    if (entries != NULL) {
        g_dir_close(entries);
    }
    return has_svg;
}

cairo_surface_t *appearance_theme_image_load(const theme_image_source *source, const char *name)
{
    cairo_surface_t *svg = source->scale > 0.0 ? load_svg(source, name) : NULL;

    return svg != NULL ? svg : load_bitmap(source, name);
}

static cairo_surface_t *load_bitmap(const theme_image_source *source, const char *name)
{
    GdkPixbuf *base = load_xpm(source, name);
    GdkPixbuf *image = compose(base, load_overlay(source->dir, name));

    if (image == NULL) {
        return NULL;
    }
    cairo_surface_t *surface = surface_from_pixbuf(image);
    g_object_unref(image);
    return surface;
}

static cairo_surface_t *load_svg(const theme_image_source *source, const char *name)
{
    char *file = g_strdup_printf("%s.%s", name, svg_extension);
    char *path = g_build_filename(source->dir, file, NULL);
    xfwmColorSymbol *colors = svg_colors(source->symbols);
    GdkPixbuf *pixbuf = g_file_test(path, G_FILE_TEST_IS_REGULAR)
                            ? xfwmSvgLoad(path, colors, source->scale)
                            : NULL;
    cairo_surface_t *surface = pixbuf != NULL ? surface_from_pixbuf(pixbuf) : NULL;

    if (pixbuf != NULL) {
        g_object_unref(pixbuf);
    }
    g_free(colors);
    g_free(path);
    g_free(file);
    return surface;
}

static xfwmColorSymbol *svg_colors(const xpm_symbols *symbols)
{
    size_t count = symbols != NULL ? symbols->count : 0;
    xfwmColorSymbol *colors = g_new0(xfwmColorSymbol, count + 1);

    for (size_t i = 0; i < count; i++) {
        colors[i] = (xfwmColorSymbol) { (gchar *) symbols->items[i].name,
                                        symbols->items[i].value };
    }
    return colors;
}

static cairo_surface_t *surface_from_pixbuf(const GdkPixbuf *pixbuf)
{
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        return surface;
    }
    cairo_surface_flush(surface);
    for (int y = 0; y < height; y++) {
        copy_row(pixbuf, y, (uint32_t *) (void *) (data + (size_t) y * (size_t) stride));
    }
    cairo_surface_mark_dirty(surface);
    return surface;
}

static GdkPixbuf *load_xpm(const theme_image_source *source, const char *name)
{
    char *file = g_strdup_printf("%s.xpm", name);
    char *path = g_build_filename(source->dir, file, NULL);
    GdkPixbuf *pixbuf = appearance_xpm_load(path, source->symbols);

    g_free(path);
    g_free(file);
    return pixbuf;
}

static GdkPixbuf *load_overlay(const char *dir, const char *name)
{
    GdkPixbuf *overlay = NULL;

    for (size_t i = 0; overlay == NULL && i < G_N_ELEMENTS(overlay_extensions); i++) {
        char *file = g_strdup_printf("%s.%s", name, overlay_extensions[i]);
        char *path = g_build_filename(dir, file, NULL);
        if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
            overlay = gdk_pixbuf_new_from_file(path, NULL);
        }
        g_free(path);
        g_free(file);
    }
    return overlay;
}

static GdkPixbuf *compose(GdkPixbuf *base, GdkPixbuf *overlay)
{
    if (overlay == NULL) {
        return base;
    }
    if (base == NULL) {
        return overlay;
    }
    int width = MIN(gdk_pixbuf_get_width(base), gdk_pixbuf_get_width(overlay));
    int height = MIN(gdk_pixbuf_get_height(base), gdk_pixbuf_get_height(overlay));
    gdk_pixbuf_composite(overlay, base, 0, 0, width, height, 0, 0, 1.0, 1.0,
                         GDK_INTERP_BILINEAR, OPAQUE);
    g_object_unref(overlay);
    return base;
}

static void copy_row(const GdkPixbuf *pixbuf, int y, uint32_t *dest)
{
    int channels = gdk_pixbuf_get_n_channels(pixbuf);
    size_t stride = (size_t) gdk_pixbuf_get_rowstride(pixbuf);
    const guchar *source = gdk_pixbuf_read_pixels(pixbuf) + (size_t) y * stride;

    for (int x = 0; x < gdk_pixbuf_get_width(pixbuf); x++) {
        dest[x] = cut_out_pixel(source + (size_t) x * (size_t) channels, channels);
    }
}

static uint32_t cut_out_pixel(const guchar *pixel, int channels)
{
    if (channels == RGBA_CHANNELS && pixel[ALPHA_CHANNEL] != OPAQUE) {
        return 0;
    }
    return ((uint32_t) OPAQUE << ALPHA_SHIFT) | ((uint32_t) pixel[RED_CHANNEL] << RED_SHIFT)
           | ((uint32_t) pixel[GREEN_CHANNEL] << GREEN_SHIFT) | pixel[BLUE_CHANNEL];
}
