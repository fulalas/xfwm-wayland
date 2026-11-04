#include <unistd.h>
#include <wlr/util/log.h>

#include "resource.h"
#include "sealed_memfd.h"
#include "xfce_toplevel_private.h"

enum {
    ICON_SCALE = 1,
    NO_FD = -1,
};

static const char FD_NAME[] = "xfwm-toplevel-icon";

static bool has_pixel_data(cairo_surface_t *pixels);
static bool same_size(icon_size offered, icon_size wanted);
static void send_pixels(struct wl_resource *answer, cairo_surface_t *pixels);
static int pixels_fd(cairo_surface_t *pixels);

static const struct xfce_foreign_toplevel_icon_pixels_v1_interface pixels_impl = {
    .destroy = resource_handle_destroy,
};

icon_size xfce_icon_size_of(cairo_surface_t *pixels)
{
    int width = cairo_image_surface_get_width(pixels);
    int height = cairo_image_surface_get_height(pixels);
    return (icon_size){(uint32_t) (width > height ? width : height), ICON_SCALE};
}

struct wl_resource *xfce_icon_pixels_create(struct wl_resource *parent, uint32_t id)
{
    struct wl_resource *answer =
        wl_resource_create(wl_resource_get_client(parent),
                           &xfce_foreign_toplevel_icon_pixels_v1_interface,
                           wl_resource_get_version(parent), id);
    if (answer != NULL) {
        wl_resource_set_implementation(answer, &pixels_impl, NULL, NULL);
    }
    return answer;
}

void xfce_icon_pixels_answer(struct wl_resource *answer, cairo_surface_t *pixels, icon_size wanted)
{
    if (pixels == NULL || !has_pixel_data(pixels)) {
        xfce_foreign_toplevel_icon_pixels_v1_send_failed(
            answer, XFCE_FOREIGN_TOPLEVEL_ICON_PIXELS_V1_FAILURE_REASON_NO_PIXEL_DATA);
        return;
    }
    if (!same_size(xfce_icon_size_of(pixels), wanted)) {
        xfce_foreign_toplevel_icon_pixels_v1_send_failed(
            answer, XFCE_FOREIGN_TOPLEVEL_ICON_PIXELS_V1_FAILURE_REASON_INVALID_ARGS);
        return;
    }
    send_pixels(answer, pixels);
}

static bool has_pixel_data(cairo_surface_t *pixels)
{
    return cairo_surface_get_type(pixels) == CAIRO_SURFACE_TYPE_IMAGE &&
           cairo_image_surface_get_format(pixels) == CAIRO_FORMAT_ARGB32;
}

static bool same_size(icon_size offered, icon_size wanted)
{
    return offered.size == wanted.size && offered.scale == wanted.scale;
}

static void send_pixels(struct wl_resource *answer, cairo_surface_t *pixels)
{
    int fd = pixels_fd(pixels);
    if (fd == NO_FD) {
        xfce_foreign_toplevel_icon_pixels_v1_send_failed(
            answer, XFCE_FOREIGN_TOPLEVEL_ICON_PIXELS_V1_FAILURE_REASON_UNKNOWN);
        return;
    }
    xfce_foreign_toplevel_icon_pixels_v1_send_pixels(
        answer, fd, (uint32_t) cairo_image_surface_get_width(pixels),
        (uint32_t) cairo_image_surface_get_height(pixels),
        (uint32_t) cairo_image_surface_get_stride(pixels));
    close(fd);
}

static int pixels_fd(cairo_surface_t *pixels)
{
    cairo_surface_flush(pixels);
    const unsigned char *data = cairo_image_surface_get_data(pixels);
    size_t length = (size_t) cairo_image_surface_get_stride(pixels) *
                    (size_t) cairo_image_surface_get_height(pixels);
    if (data == NULL) {
        wlr_log(WLR_ERROR, "cannot read the icon pixels");
        return NO_FD;
    }
    return sealed_memfd_create(FD_NAME, data, length);
}

