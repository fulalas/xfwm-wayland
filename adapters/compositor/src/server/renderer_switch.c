#include "renderer_switch.h"

#include <stdlib.h>
#include <wlr/render/allocator.h>
#include <wlr/render/egl.h>
#include <wlr/render/gles2.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/util/log.h>

#include "render/client_textures.h"
#include "util/listener.h"
#include "output/output.h"
#include "output/output_color.h"
#include "protocols.h"
#include "input/seat.h"
#include "toplevel/toplevel_capture.h"

typedef struct render_pair {
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
} render_pair;

static renderer_kind wanted_kind(const compositor *server);
static bool can_switch_to(const compositor *server, renderer_kind kind);
static bool switch_to(compositor *server, renderer_kind kind);
static const char *kind_name(renderer_kind kind);
static bool create_pair(compositor *server, renderer_kind kind, render_pair *pair);
static struct wlr_renderer *create_renderer(compositor *server, renderer_kind kind);
static bool replace_renderer(compositor *server, renderer_kind kind);
static bool swap_in(compositor *server, const render_pair *pair, renderer_kind kind);
static bool use_pair(compositor *server, const render_pair *pair, renderer_kind kind);
static void adopt_pair(compositor *server, const render_pair *pair, renderer_kind kind);
static void destroy_pair(const render_pair *pair);
static void keep_context_current(struct wlr_renderer *renderer);
static void redraw_everything(compositor *server);
static void watch_lost(compositor *server);
static void handle_lost(struct wl_listener *listener, void *data);
static void recover_from_reset(void *data);

bool renderer_create_initial(compositor *c)
{
    c->render.forced = getenv("WLR_RENDERER") != NULL;
    wl_list_init(&c->render.lost.link);
    render_pair pair = {0};
    renderer_kind kind = wanted_kind(c);
    if (!create_pair(c, kind, &pair) && kind == RENDERER_KIND_VULKAN) {
        wlr_log(WLR_ERROR, "cannot start the Vulkan renderer needed for HDR, HDR stays off");
        c->render.failed = kind;
        kind = RENDERER_KIND_DEFAULT;
        create_pair(c, kind, &pair);
    }
    adopt_pair(c, &pair, kind);
    keep_context_current(c->renderer);
    watch_lost(c);
    return pair.allocator != NULL;
}

void renderer_follow_config(compositor *c)
{
    renderer_kind wanted = wanted_kind(c);
    if (wanted == c->render.current) {
        c->render.failed = RENDERER_KIND_NONE;
    }
    if (!can_switch_to(c, wanted) || !switch_to(c, wanted)) {
        outputs_apply_color(c->outputs);
    }
}

bool renderer_fall_back(compositor *c)
{
    if (c->render.current != RENDERER_KIND_VULKAN) {
        return false;
    }
    wlr_log(WLR_ERROR, "the vulkan renderer cannot drive the screens, switching back");
    c->render.failed = RENDERER_KIND_VULKAN;
    return replace_renderer(c, RENDERER_KIND_DEFAULT);
}

void renderer_keep_ready(compositor *c)
{
    keep_context_current(c->renderer);
}

void renderer_release(compositor *c)
{
    listener_disconnect(&c->render.lost);
}

static renderer_kind wanted_kind(const compositor *c)
{
    const wm_compositing_config *config = &c->config.compositing;
    if (c->render.forced) {
        return RENDERER_KIND_DEFAULT;
    }
    if (!config->use_gl) {
        return RENDERER_KIND_PIXMAN;
    }
    return config->hdr ? RENDERER_KIND_VULKAN : RENDERER_KIND_DEFAULT;
}

static bool can_switch_to(const compositor *c, renderer_kind kind)
{
    renderer_kind current = c->render.current;
    return kind != current && kind != c->render.failed && kind != RENDERER_KIND_PIXMAN &&
           current != RENDERER_KIND_PIXMAN;
}

static bool switch_to(compositor *c, renderer_kind kind)
{
    renderer_kind previous = c->render.current;
    if (replace_renderer(c, kind) && outputs_rebuild_swapchains(c->outputs) &&
        outputs_apply_color(c->outputs)) {
        return true;
    }
    wlr_log(WLR_ERROR, "the %s renderer cannot drive the screens, switching back", kind_name(kind));
    c->render.failed = kind;
    if (c->render.current != previous) {
        replace_renderer(c, previous);
    }
    return false;
}

static const char *kind_name(renderer_kind kind)
{
    switch (kind) {
    case RENDERER_KIND_VULKAN:
        return "vulkan";
    case RENDERER_KIND_PIXMAN:
        return "pixman";
    case RENDERER_KIND_DEFAULT:
        return "default";
    default:
        return "none";
    }
}

static bool create_pair(compositor *c, renderer_kind kind, render_pair *pair)
{
    pair->renderer = create_renderer(c, kind);
    pair->allocator =
        pair->renderer != NULL ? wlr_allocator_autocreate(c->backend, pair->renderer) : NULL;
    if (pair->allocator == NULL && pair->renderer != NULL) {
        wlr_renderer_destroy(pair->renderer);
        pair->renderer = NULL;
    }
    return pair->allocator != NULL;
}

static struct wlr_renderer *create_renderer(compositor *c, renderer_kind kind)
{
    if (kind == RENDERER_KIND_DEFAULT) {
        return wlr_renderer_autocreate(c->backend);
    }
    setenv("WLR_RENDERER", kind_name(kind), 1);
    struct wlr_renderer *renderer = wlr_renderer_autocreate(c->backend);
    unsetenv("WLR_RENDERER");
    return renderer;
}

static bool replace_renderer(compositor *c, renderer_kind kind)
{
    render_pair pair = {0};
    if (!create_pair(c, kind, &pair)) {
        wlr_log(WLR_ERROR, "cannot start the %s renderer", kind_name(kind));
        keep_context_current(c->renderer);
        return false;
    }
    return swap_in(c, &pair, kind);
}

static bool swap_in(compositor *c, const render_pair *pair, renderer_kind kind)
{
    render_pair old = {c->renderer, c->allocator};
    renderer_kind old_kind = c->render.current;
    toplevel_capture_drop_sources(c->toplevel_capture);
    listener_disconnect(&c->render.lost);
    bool ok = use_pair(c, pair, kind);
    if (!ok) {
        use_pair(c, &old, old_kind);
    }
    destroy_pair(ok ? &old : pair);
    keep_context_current(c->renderer);
    watch_lost(c);
    protocols_create_color(c);
    redraw_everything(c);
    return ok;
}

static bool use_pair(compositor *c, const render_pair *pair, renderer_kind kind)
{
    adopt_pair(c, pair, kind);
    wlr_compositor_set_renderer(c->globals.compositor, pair->renderer);
    return outputs_renderer_changed(c->outputs);
}

static void adopt_pair(compositor *c, const render_pair *pair, renderer_kind kind)
{
    c->renderer = pair->renderer;
    c->allocator = pair->allocator;
    c->render.current = kind;
}

static void keep_context_current(struct wlr_renderer *renderer)
{
    if (renderer == NULL || !wlr_renderer_is_gles2(renderer)) {
        return;
    }
    struct wlr_egl *egl = wlr_gles2_renderer_get_egl(renderer);
    if (eglGetCurrentContext() == wlr_egl_get_context(egl)) {
        return;
    }
    if (!eglMakeCurrent(wlr_egl_get_display(egl), EGL_NO_SURFACE, EGL_NO_SURFACE,
                        wlr_egl_get_context(egl))) {
        wlr_log(WLR_ERROR, "cannot keep the OpenGL context current");
    }
}

static void destroy_pair(const render_pair *pair)
{
    wlr_allocator_destroy(pair->allocator);
    wlr_renderer_destroy(pair->renderer);
}

static void redraw_everything(compositor *c)
{
    client_textures_rebuild(c->display, c->renderer);
    compositor_appearance_changed(c);
    seat_refresh_cursor(c->seat);
}

static void watch_lost(compositor *c)
{
    if (c->renderer != NULL) {
        listener_connect(&c->render.lost, &c->renderer->events.lost, handle_lost);
    }
}

static void handle_lost(struct wl_listener *listener, void *data)
{
    (void) data;
    compositor *c = wl_container_of(listener, c, render.lost);
    wlr_log(WLR_ERROR, "GPU reset: recreating the renderer");
    listener_disconnect(&c->render.lost);
    wl_event_loop_add_idle(c->loop, recover_from_reset, c);
}

static void recover_from_reset(void *data)
{
    compositor *c = data;
    if (!replace_renderer(c, c->render.current)) {
        wlr_log(WLR_ERROR, "cannot recover from the GPU reset");
    }
}
