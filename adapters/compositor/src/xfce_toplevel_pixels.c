#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wlr/util/log.h>

#include "xfce_toplevel_private.h"

enum {
    ICON_SCALE = 1,
    NO_FD = -1,
};

static const char FD_NAME[] = "xfwm-toplevel-icon";
static const unsigned SEALS = F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_SEAL;

static void handle_destroy(struct wl_client *client, struct wl_resource *resource);
static bool has_pixel_data(cairo_surface_t *pixels);
static bool same_size(icon_size offered, icon_size wanted);
static void send_pixels(struct wl_resource *answer, cairo_surface_t *pixels);
static int pixels_fd(cairo_surface_t *pixels);
static bool write_all(int fd, const unsigned char *data, size_t length);

static const struct xfce_foreign_toplevel_icon_pixels_v1_interface pixels_impl = {
    .destroy = handle_destroy,
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

static void handle_destroy(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    wl_resource_destroy(resource);
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
    int fd = data != NULL ? memfd_create(FD_NAME, MFD_CLOEXEC | MFD_ALLOW_SEALING) : NO_FD;
    if (fd == NO_FD) {
        wlr_log(WLR_ERROR, "cannot create the icon pixel buffer: %s", strerror(errno));
        return NO_FD;
    }
    if (!write_all(fd, data, length) || fcntl(fd, F_ADD_SEALS, SEALS) != 0) {
        wlr_log(WLR_ERROR, "cannot fill the icon pixel buffer: %s", strerror(errno));
        close(fd);
        return NO_FD;
    }
    return fd;
}

static bool write_all(int fd, const unsigned char *data, size_t length)
{
    while (length > 0) {
        ssize_t written = write(fd, data, length);
        if (written <= 0) {
            return false;
        }
        data += written;
        length -= (size_t) written;
    }
    return true;
}
