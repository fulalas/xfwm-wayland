#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_linux_drm_syncobj_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "util/listener.h"
#include "osd_preview.h"

struct osd_preview {
    compositor *server;
    struct wlr_scene_buffer *node;
    struct wlr_surface *surface;
    struct wl_listener sample;
    struct wl_listener surface_destroy;
};

static void track_surface(osd_preview *preview, struct wlr_surface *surface);
static struct wlr_scene_buffer_set_buffer_options acquire_options(struct wlr_surface *surface);
static bool shows_current_buffer(const osd_preview *preview);
static void handle_sample(struct wl_listener *listener, void *data);
static void handle_surface_destroy(struct wl_listener *listener, void *data);

osd_preview *osd_preview_create(compositor *server, struct wlr_scene_tree *parent)
{
    osd_preview *preview = calloc(1, sizeof(*preview));
    if (preview == NULL) {
        return NULL;
    }
    preview->server = server;
    preview->node = wlr_scene_buffer_create(parent, NULL);
    if (preview->node == NULL) {
        free(preview);
        return NULL;
    }
    wl_list_init(&preview->surface_destroy.link);
    listener_connect(&preview->sample, &preview->node->events.output_sample, handle_sample);
    return preview;
}

void osd_preview_destroy(osd_preview *preview)
{
    track_surface(preview, NULL);
    listener_disconnect(&preview->sample);
    wlr_scene_node_destroy(&preview->node->node);
    free(preview);
}

bool osd_preview_show(osd_preview *preview, struct wlr_surface *surface)
{
    if (surface == NULL || surface->buffer == NULL) {
        osd_preview_clear(preview);
        return false;
    }
    struct wlr_scene_buffer_set_buffer_options options = acquire_options(surface);
    struct wlr_fbox source;
    wlr_surface_get_buffer_source_box(surface, &source);
    track_surface(preview, surface);
    wlr_scene_buffer_set_buffer_with_options(preview->node, &surface->buffer->base, &options);
    wlr_scene_buffer_set_source_box(preview->node, &source);
    wlr_scene_buffer_set_transform(preview->node, surface->current.transform);
    return true;
}

void osd_preview_clear(osd_preview *preview)
{
    track_surface(preview, NULL);
    wlr_scene_buffer_set_buffer(preview->node, NULL);
}

struct wlr_scene_buffer *osd_preview_node(const osd_preview *preview)
{
    return preview->node;
}

static void track_surface(osd_preview *preview, struct wlr_surface *surface)
{
    if (preview->surface == surface) {
        return;
    }
    listener_disconnect(&preview->surface_destroy);
    preview->surface = surface;
    if (surface != NULL) {
        listener_connect(&preview->surface_destroy, &surface->events.destroy,
                         handle_surface_destroy);
    }
}

static struct wlr_scene_buffer_set_buffer_options acquire_options(struct wlr_surface *surface)
{
    struct wlr_linux_drm_syncobj_surface_v1_state *sync =
        wlr_linux_drm_syncobj_v1_get_surface_state(surface);
    if (sync == NULL) {
        return (struct wlr_scene_buffer_set_buffer_options){0};
    }
    return (struct wlr_scene_buffer_set_buffer_options){
        .wait_timeline = sync->acquire_timeline,
        .wait_point = sync->acquire_point,
    };
}

static bool shows_current_buffer(const osd_preview *preview)
{
    struct wlr_surface *surface = preview->surface;
    return surface != NULL && surface->buffer != NULL &&
           preview->node->buffer == &surface->buffer->base;
}

static void handle_sample(struct wl_listener *listener, void *data)
{
    osd_preview *preview = wl_container_of(listener, preview, sample);
    const struct wlr_scene_output_sample_event *event = data;
    if (event->release_timeline == NULL || !shows_current_buffer(preview)) {
        return;
    }
    struct wlr_linux_drm_syncobj_surface_v1_state *sync =
        wlr_linux_drm_syncobj_v1_get_surface_state(preview->surface);
    if (sync != NULL &&
        !wlr_linux_drm_syncobj_v1_state_add_release_point(sync, event->release_timeline,
                                                          event->release_point,
                                                          preview->server->loop)) {
        wlr_log(WLR_ERROR, "cannot add a release point for an Alt+Tab preview");
    }
}

static void handle_surface_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    osd_preview *preview = wl_container_of(listener, preview, surface_destroy);
    osd_preview_clear(preview);
}
