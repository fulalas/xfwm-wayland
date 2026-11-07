#include "client_textures.h"

#include <wayland-server-core.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_compositor.h>

#include "client_surfaces.h"

static void rebuild_surface(struct wlr_surface *surface, void *data);
static void rebuild_texture(struct wlr_client_buffer *buffer, struct wlr_renderer *renderer);

void client_textures_rebuild(struct wl_display *display, struct wlr_renderer *renderer)
{
    client_surfaces_for_each(display, rebuild_surface, renderer);
}

static void rebuild_surface(struct wlr_surface *surface, void *data)
{
    if (surface->buffer != NULL) {
        rebuild_texture(surface->buffer, data);
    }
}

static void rebuild_texture(struct wlr_client_buffer *buffer, struct wlr_renderer *renderer)
{
    if (buffer->texture != NULL || buffer->source == NULL) {
        return;
    }
    buffer->texture = wlr_texture_from_buffer(renderer, buffer->source);
    if (buffer->texture != NULL) {
        struct wl_listener *gone = &buffer->WLR_PRIVATE.renderer_destroy;
        wl_list_remove(&gone->link);
        wl_signal_add(&renderer->events.destroy, gone);
    }
}
