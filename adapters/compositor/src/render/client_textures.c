#include "client_textures.h"

#include <drm_fourcc.h>
#include <pixman.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_compositor.h>

#include "client_surfaces.h"

enum {
    COPY_BYTES_PER_PIXEL = 4,
};

static void move_surface(struct wlr_surface *surface, void *data);
static struct wlr_texture *texture_for(struct wlr_surface *surface,
                                       struct wlr_renderer *renderer);
static struct wlr_texture *copy_texture(struct wlr_texture *texture,
                                        struct wlr_renderer *renderer, bool opaque);
static bool is_opaque(struct wlr_surface *surface);
static void adopt_texture(struct wlr_client_buffer *buffer, struct wlr_texture *texture);

void client_textures_move(struct wl_display *display, struct wlr_renderer *renderer)
{
    client_surfaces_for_each(display, move_surface, renderer);
}

static void move_surface(struct wlr_surface *surface, void *data)
{
    struct wlr_renderer *renderer = data;
    struct wlr_client_buffer *buffer = surface->buffer;
    if (buffer == NULL || (buffer->texture != NULL && buffer->texture->renderer == renderer)) {
        return;
    }
    struct wlr_texture *texture = texture_for(surface, renderer);
    if (texture != NULL) {
        adopt_texture(buffer, texture);
    }
}

static struct wlr_texture *texture_for(struct wlr_surface *surface,
                                       struct wlr_renderer *renderer)
{
    struct wlr_client_buffer *buffer = surface->buffer;
    if (buffer->source != NULL) {
        return wlr_texture_from_buffer(renderer, buffer->source);
    }
    if (buffer->texture == NULL) {
        return NULL;
    }
    return copy_texture(buffer->texture, renderer, is_opaque(surface));
}

static struct wlr_texture *copy_texture(struct wlr_texture *texture,
                                        struct wlr_renderer *renderer, bool opaque)
{
    uint32_t stride = texture->width * COPY_BYTES_PER_PIXEL;
    void *pixels = malloc((size_t) stride * texture->height);
    if (pixels == NULL) {
        return NULL;
    }
    struct wlr_texture_read_pixels_options options = {
        .data = pixels,
        .format = DRM_FORMAT_ABGR8888,
        .stride = stride,
    };
    struct wlr_texture *copy = NULL;
    if (wlr_texture_read_pixels(texture, &options)) {
        uint32_t format = opaque ? DRM_FORMAT_XBGR8888 : DRM_FORMAT_ABGR8888;
        copy = wlr_texture_from_pixels(renderer, format, stride, texture->width, texture->height,
                                       pixels);
    }
    free(pixels);
    return copy;
}

static bool is_opaque(struct wlr_surface *surface)
{
    pixman_box32_t whole = {0, 0, surface->current.width, surface->current.height};
    return pixman_region32_contains_rectangle(&surface->opaque_region, &whole) == PIXMAN_REGION_IN;
}

static void adopt_texture(struct wlr_client_buffer *buffer, struct wlr_texture *texture)
{
    struct wl_listener *gone = &buffer->WLR_PRIVATE.renderer_destroy;
    wlr_texture_destroy(buffer->texture);
    buffer->texture = texture;
    wl_list_remove(&gone->link);
    wl_signal_add(&texture->renderer->events.destroy, gone);
}
