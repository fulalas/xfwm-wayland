#include <drm_fourcc.h>
#include <wlr/types/wlr_buffer.h>

#include "cairo_buffer.h"
#include "icon_convert.h"
#include "icon_pixels.h"

enum {
    EWMH_ICONS_MAX = 16,
    OPAQUE_ALPHA = 0xff000000u,
};

typedef struct ewmh_icon {
    wm_size size;
    const uint32_t *pixels;
} ewmh_icon;

typedef uint32_t (*pixel_map)(uint32_t argb);

static size_t collect_icons(const xcb_ewmh_get_wm_icon_reply_t *reply, ewmh_icon *icons);
static cairo_surface_t *premultiplied_surface(const ewmh_icon *icon);
static void map_pixels(cairo_surface_t *surface, pixel_map map);
static uint32_t opaque(uint32_t argb);

cairo_surface_t *icon_convert_buffer(struct wlr_buffer *buffer)
{
    void *data = NULL;
    uint32_t format = 0;
    size_t stride = 0;
    if (!wlr_buffer_begin_data_ptr_access(buffer, WLR_BUFFER_DATA_PTR_ACCESS_READ, &data, &format,
                                          &stride)) {
        return NULL;
    }
    cairo_surface_t *surface = NULL;
    if (format == DRM_FORMAT_ARGB8888 || format == DRM_FORMAT_XRGB8888) {
        pixel_rows rows = {data, buffer->width, buffer->height, (int) stride};
        surface = cairo_buffer_copy_pixels(&rows);
    }
    wlr_buffer_end_data_ptr_access(buffer);
    if (surface != NULL && format == DRM_FORMAT_XRGB8888) {
        map_pixels(surface, opaque);
    }
    return surface;
}

cairo_surface_t *icon_convert_ewmh(const xcb_ewmh_get_wm_icon_reply_t *reply, int wanted_size)
{
    ewmh_icon icons[EWMH_ICONS_MAX];
    size_t count = collect_icons(reply, icons);
    if (count == 0) {
        return NULL;
    }
    wm_size sizes[EWMH_ICONS_MAX];
    for (size_t i = 0; i < count; i++) {
        sizes[i] = icons[i].size;
    }
    return premultiplied_surface(&icons[icon_pixels_best_index(sizes, count, wanted_size)]);
}

static size_t collect_icons(const xcb_ewmh_get_wm_icon_reply_t *reply, ewmh_icon *icons)
{
    size_t count = 0;
    xcb_ewmh_wm_icon_iterator_t iterator =
        xcb_ewmh_get_wm_icon_iterator((xcb_ewmh_get_wm_icon_reply_t *) reply);
    while (iterator.rem > 0 && count < EWMH_ICONS_MAX) {
        if (iterator.width > 0 && iterator.height > 0) {
            icons[count++] = (ewmh_icon){{(int) iterator.width, (int) iterator.height},
                                         iterator.data};
        }
        xcb_ewmh_get_wm_icon_next(&iterator);
    }
    return count;
}

static cairo_surface_t *premultiplied_surface(const ewmh_icon *icon)
{
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, icon->size.width);
    if (stride < 0) {
        return NULL;
    }
    pixel_rows rows = {icon->pixels, icon->size.width, icon->size.height, stride};
    cairo_surface_t *surface = cairo_buffer_copy_pixels(&rows);
    if (surface != NULL) {
        map_pixels(surface, icon_pixels_premultiply);
    }
    return surface;
}

static void map_pixels(cairo_surface_t *surface, pixel_map map)
{
    cairo_surface_flush(surface);
    unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    int width = cairo_image_surface_get_width(surface);
    for (int y = 0; y < cairo_image_surface_get_height(surface); y++) {
        uint32_t *row = (uint32_t *) (data + (size_t) y * stride);
        for (int x = 0; x < width; x++) {
            row[x] = map(row[x]);
        }
    }
    cairo_surface_mark_dirty(surface);
}

static uint32_t opaque(uint32_t argb)
{
    return argb | OPAQUE_ALPHA;
}
