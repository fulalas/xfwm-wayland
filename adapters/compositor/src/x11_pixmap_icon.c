#include "x11_pixmap_icon.h"

#include <stdlib.h>

enum {
    BITS_PER_BYTE = 8,
    BYTES_PER_PIXEL = 4,
    ICON_SIZE_MAX = 256,
    DEPTH_BITMAP = 1,
    DEPTH_ALPHA = 32,
};

static const uint32_t OPAQUE_BLACK = 0xff000000u;
static const uint32_t COLOR_MASK = 0x00ffffffu;
static const uint32_t WHITE = 0xffffffffu;

typedef struct fetched_image {
    xcb_get_image_reply_t *reply;
    uint8_t depth;
    int width;
    int height;
} fetched_image;

static bool bit_at(const x11_bitmap *bitmap, int x, int y);
static uint32_t color_at(const x11_icon_pixels *pixels, int x, int y);
static bool fetch(xcb_connection_t *connection, xcb_pixmap_t pixmap, fetched_image *image);
static x11_bitmap bitmap_of(xcb_connection_t *connection, const fetched_image *image);

cairo_surface_t *x11_icon_from_pixels(const x11_icon_pixels *pixels)
{
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixels->width, pixels->height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        return NULL;
    }
    uint8_t *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    for (int y = 0; y < pixels->height; y++) {
        uint32_t *row = (uint32_t *) (data + (size_t) y * (size_t) stride);
        for (int x = 0; x < pixels->width; x++) {
            bool shown = pixels->mask == NULL || bit_at(pixels->mask, x, y);
            row[x] = shown ? color_at(pixels, x, y) : 0;
        }
    }
    cairo_surface_mark_dirty(surface);
    return surface;
}

cairo_surface_t *x11_pixmap_icon(xcb_connection_t *connection, xcb_pixmap_t pixmap,
                                 xcb_pixmap_t mask)
{
    fetched_image image = {0};
    fetched_image mask_image = {0};
    if (!fetch(connection, pixmap, &image)) {
        return NULL;
    }
    bool has_mask = mask != XCB_NONE && fetch(connection, mask, &mask_image) &&
                    mask_image.depth == DEPTH_BITMAP && mask_image.width >= image.width &&
                    mask_image.height >= image.height;
    x11_bitmap mask_bits = has_mask ? bitmap_of(connection, &mask_image) : (x11_bitmap){0};
    x11_icon_pixels pixels = {
        .width = image.width,
        .height = image.height,
        .depth = image.depth,
        .colors = (const uint32_t *) xcb_get_image_data(image.reply),
        .color_stride = (uint32_t) image.width,
        .bits = bitmap_of(connection, &image),
        .mask = has_mask ? &mask_bits : NULL,
    };
    int needed = image.width * image.height * BYTES_PER_PIXEL;
    bool usable =
        image.depth == DEPTH_BITMAP || xcb_get_image_data_length(image.reply) >= needed;
    cairo_surface_t *surface = usable ? x11_icon_from_pixels(&pixels) : NULL;
    free(image.reply);
    free(mask_image.reply);
    return surface;
}

static bool bit_at(const x11_bitmap *bitmap, int x, int y)
{
    uint8_t byte = bitmap->data[(size_t) y * bitmap->stride + (size_t) x / BITS_PER_BYTE];
    int bit = x % BITS_PER_BYTE;
    return (byte >> (bitmap->lsb_first ? bit : BITS_PER_BYTE - 1 - bit)) & 1;
}

static uint32_t color_at(const x11_icon_pixels *pixels, int x, int y)
{
    if (pixels->depth == DEPTH_BITMAP) {
        return bit_at(&pixels->bits, x, y) ? OPAQUE_BLACK : WHITE;
    }
    uint32_t color = pixels->colors[(size_t) y * pixels->color_stride + (size_t) x];
    return pixels->depth == DEPTH_ALPHA ? color : OPAQUE_BLACK | (color & COLOR_MASK);
}

static bool fetch(xcb_connection_t *connection, xcb_pixmap_t pixmap, fetched_image *image)
{
    xcb_generic_error_t *error = NULL;
    xcb_get_geometry_reply_t *geometry =
        xcb_get_geometry_reply(connection, xcb_get_geometry(connection, pixmap), &error);
    free(error);
    if (geometry == NULL) {
        return false;
    }
    *image = (fetched_image){NULL, geometry->depth, geometry->width, geometry->height};
    free(geometry);
    if (image->width <= 0 || image->height <= 0 || image->width > ICON_SIZE_MAX ||
        image->height > ICON_SIZE_MAX) {
        return false;
    }
    uint8_t format = image->depth == DEPTH_BITMAP ? XCB_IMAGE_FORMAT_XY_PIXMAP
                                                  : XCB_IMAGE_FORMAT_Z_PIXMAP;
    xcb_get_image_cookie_t cookie = xcb_get_image(connection, format, pixmap, 0, 0,
                                                  (uint16_t) image->width,
                                                  (uint16_t) image->height, UINT32_MAX);
    error = NULL;
    image->reply = xcb_get_image_reply(connection, cookie, &error);
    free(error);
    return image->reply != NULL;
}

static x11_bitmap bitmap_of(xcb_connection_t *connection, const fetched_image *image)
{
    const xcb_setup_t *setup = xcb_get_setup(connection);
    uint32_t pad = setup->bitmap_format_scanline_pad;
    uint32_t stride = ((uint32_t) image->width + pad - 1) / pad * pad / BITS_PER_BYTE;
    bool lsb_first = setup->bitmap_format_bit_order == XCB_IMAGE_ORDER_LSB_FIRST;
    return (x11_bitmap){xcb_get_image_data(image->reply), stride, lsb_first};
}
