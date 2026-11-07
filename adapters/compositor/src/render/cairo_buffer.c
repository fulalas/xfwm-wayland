#include <drm_fourcc.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/interfaces/wlr_buffer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "cairo_buffer.h"

typedef struct cairo_buffer {
    struct wlr_buffer base;
    cairo_surface_t *surface;
} cairo_buffer;

static bool attach(struct wlr_scene_buffer *node, cairo_surface_t *surface);
static cairo_surface_t *image_surface_for(cairo_surface_t *surface);
static cairo_surface_t *convert_to_argb(cairo_surface_t *surface);
static void buffer_destroy(struct wlr_buffer *buffer);
static bool buffer_begin_access(struct wlr_buffer *buffer, uint32_t flags, void **data,
                                uint32_t *format, size_t *stride);
static void buffer_end_access(struct wlr_buffer *buffer);

static const struct wlr_buffer_impl cairo_buffer_impl = {
    .destroy = buffer_destroy,
    .begin_data_ptr_access = buffer_begin_access,
    .end_data_ptr_access = buffer_end_access,
};

struct wlr_buffer *cairo_buffer_create(cairo_surface_t *surface)
{
    cairo_surface_t *image = image_surface_for(surface);
    if (image == NULL) {
        return NULL;
    }
    cairo_buffer *buffer = calloc(1, sizeof(*buffer));
    if (buffer == NULL) {
        cairo_surface_destroy(image);
        return NULL;
    }
    buffer->surface = image;
    wlr_buffer_init(&buffer->base, &cairo_buffer_impl, cairo_image_surface_get_width(image),
                    cairo_image_surface_get_height(image));
    return &buffer->base;
}

struct wlr_buffer *cairo_buffer_create_solid(uint32_t argb)
{
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_surface_flush(surface);
    uint32_t *pixel = (uint32_t *) cairo_image_surface_get_data(surface);
    if (pixel != NULL) {
        *pixel = argb;
    }
    cairo_surface_mark_dirty(surface);
    struct wlr_buffer *buffer = cairo_buffer_create(surface);
    cairo_surface_destroy(surface);
    return buffer;
}

bool cairo_buffer_attach_sized(struct wlr_scene_buffer *node, cairo_surface_t *surface,
                               wm_size size)
{
    bool attached = attach(node, surface);
    if (surface != NULL) {
        cairo_surface_destroy(surface);
    }
    wlr_scene_buffer_set_dest_size(node, size.width, size.height);
    return attached;
}

cairo_surface_t *cairo_buffer_copy_pixels(const pixel_rows *rows)
{
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rows->width, rows->height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        return NULL;
    }
    cairo_surface_flush(surface);
    unsigned char *dest = cairo_image_surface_get_data(surface);
    int dest_stride = cairo_image_surface_get_stride(surface);
    size_t row_bytes = (size_t) rows->width * sizeof(uint32_t);
    const unsigned char *source = rows->data;
    for (int y = 0; y < rows->height; y++) {
        memcpy(dest + (size_t) y * dest_stride, source + (size_t) y * rows->stride, row_bytes);
    }
    cairo_surface_mark_dirty(surface);
    return surface;
}

static bool attach(struct wlr_scene_buffer *node, cairo_surface_t *surface)
{
    struct wlr_buffer *buffer = surface != NULL ? cairo_buffer_create(surface) : NULL;
    wlr_scene_buffer_set_buffer(node, buffer);
    if (buffer == NULL) {
        return false;
    }
    wlr_buffer_drop(buffer);
    return true;
}

static cairo_surface_t *image_surface_for(cairo_surface_t *surface)
{
    if (surface == NULL || cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        return NULL;
    }
    cairo_surface_flush(surface);
    if (cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_IMAGE &&
        cairo_image_surface_get_format(surface) == CAIRO_FORMAT_ARGB32) {
        return cairo_surface_reference(surface);
    }
    return convert_to_argb(surface);
}

static cairo_surface_t *convert_to_argb(cairo_surface_t *surface)
{
    double x1 = 0;
    double y1 = 0;
    double x2 = 0;
    double y2 = 0;
    cairo_t *probe = cairo_create(surface);
    cairo_clip_extents(probe, &x1, &y1, &x2, &y2);
    cairo_destroy(probe);
    cairo_surface_t *image =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int) (x2 - x1), (int) (y2 - y1));
    cairo_t *cr = cairo_create(image);
    cairo_set_source_surface(cr, surface, -x1, -y1);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_flush(image);
    return image;
}

static void buffer_destroy(struct wlr_buffer *base)
{
    cairo_buffer *buffer = wl_container_of(base, buffer, base);
    wlr_buffer_finish(base);
    cairo_surface_destroy(buffer->surface);
    free(buffer);
}

static bool buffer_begin_access(struct wlr_buffer *base, uint32_t flags, void **data,
                                uint32_t *format, size_t *stride)
{
    cairo_buffer *buffer = wl_container_of(base, buffer, base);
    if ((flags & WLR_BUFFER_DATA_PTR_ACCESS_WRITE) != 0) {
        return false;
    }
    *data = cairo_image_surface_get_data(buffer->surface);
    *format = DRM_FORMAT_ARGB8888;
    *stride = (size_t) cairo_image_surface_get_stride(buffer->surface);
    return *data != NULL;
}

static void buffer_end_access(struct wlr_buffer *base)
{
    (void) base;
}
