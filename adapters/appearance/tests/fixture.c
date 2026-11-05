#include <stdint.h>
#include "fixture.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

static const char dir_template[] = "/tmp/xfwm-appearance-test-XXXXXX";

enum {
    OPAQUE_RED = 0xFF0000FF,
};

char *fixture_dir_create(void)
{
    char *dir = g_strdup(dir_template);

    return g_mkdtemp(dir);
}

void fixture_dir_remove(char *dir)
{
    GDir *handle = g_dir_open(dir, 0, NULL);
    const char *name;

    while (handle != NULL && (name = g_dir_read_name(handle)) != NULL) {
        char *path = g_build_filename(dir, name, NULL);
        g_remove(path);
        g_free(path);
    }
    if (handle != NULL) {
        g_dir_close(handle);
    }
    g_rmdir(dir);
    g_free(dir);
}

char *fixture_write(const char *dir, const char *name, const char *content)
{
    char *path = g_build_filename(dir, name, NULL);

    g_file_set_contents(path, content, -1, NULL);
    return path;
}

bool fixture_write_png_pixel(const char *dir, const char *name, uint32_t rgba)
{
    char *path = g_build_filename(dir, name, NULL);
    GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);

    gdk_pixbuf_fill(pixbuf, rgba);
    bool saved = gdk_pixbuf_save(pixbuf, path, "png", NULL, NULL);
    g_object_unref(pixbuf);
    g_free(path);
    return saved;
}

void fixture_write_png(const char *dir, const char *name, int width, int height)
{
    char *path = g_build_filename(dir, name, NULL);
    GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);

    gdk_pixbuf_fill(pixbuf, OPAQUE_RED);
    gdk_pixbuf_save(pixbuf, path, "png", NULL, NULL);
    g_object_unref(pixbuf);
    g_free(path);
}

unsigned fixture_alpha_at(cairo_surface_t *surface, int x, int y)
{
    enum { ALPHA_SHIFT = 24 };
    const unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    const uint32_t *row = (const uint32_t *) (const void *) (data + y * stride);

    return row[x] >> ALPHA_SHIFT;
}
