#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "listener.h"
#include "osd.h"
#include "output.h"
#include "scene_layers.h"
#include "seat.h"
#include "server_modules.h"
#include "shadow_cache.h"
#include "view.h"
#include "xwayland.h"

static bool ports_are_complete(const compositor_options *options);
static void init_lists(compositor *server);
static bool painter_is_complete(const compositor_painter *painter);
static bool services_are_complete(const compositor_services *services);
static bool create_display(compositor *server);
static bool create_backend(compositor *server);
static bool create_scene(compositor *server);
static void destroy_backend(compositor *server);
static void apply_scanout(compositor *server);
static void handle_backend_destroy(struct wl_listener *listener, void *data);
static int handle_timer(void *data);

compositor *compositor_create(const compositor_options *options)
{
    if (!ports_are_complete(options)) {
        return NULL;
    }
    compositor *c = calloc(1, sizeof(*c));
    if (c == NULL) {
        return NULL;
    }
    c->painter = options->painter;
    c->services = options->services;
    c->listener = options->listener;
    c->config = options->config != NULL ? *options->config : wm_config_defaults();
    init_lists(c);
    if (!create_display(c) || !create_backend(c) || !create_scene(c) || !server_modules_create(c)) {
        wlr_log(WLR_ERROR, "cannot create the compositor");
        compositor_destroy(c);
        return NULL;
    }
    return c;
}

void compositor_destroy(compositor *c)
{
    if (c->display != NULL) {
        wl_display_destroy_clients(c->display);
    }
    server_modules_destroy(c);
    destroy_backend(c);
    if (c->display != NULL) {
        wl_display_destroy(c->display);
    }
    free(c);
}

bool compositor_start(compositor *c, wm_desktop *desktop)
{
    c->desktop = desktop;
    if (!wlr_backend_start(c->backend)) {
        wlr_log(WLR_ERROR, "cannot start the backend");
        return false;
    }
    outputs_schedule_arrange(c->outputs);
    if (c->listener != NULL && c->listener->started != NULL) {
        c->listener->started(c->listener->self);
    }
    return true;
}

void compositor_set_config(compositor *c, const wm_config *config)
{
    c->config = *config;
    apply_scanout(c);
    outputs_schedule_frames(c->outputs);
}

void compositor_appearance_changed(compositor *c)
{
    views_invalidate_decorations(c);
    osd_repaint(c->osd);
}

void compositor_input_prefs_changed(compositor *c)
{
    seat_reload_prefs(c->seat);
}

int compositor_event_fd(compositor *c)
{
    return wl_event_loop_get_fd(c->loop);
}

void compositor_dispatch(compositor *c)
{
    wl_event_loop_dispatch(c->loop, 0);
}

void compositor_flush(compositor *c)
{
    wl_event_loop_dispatch_idle(c->loop);
    wl_display_flush_clients(c->display);
}

const char *compositor_socket_name(compositor *c)
{
    return c->socket;
}

const char *compositor_x11_display(compositor *c)
{
    return xwayland_shell_display_name(c->xwayland);
}

static bool ports_are_complete(const compositor_options *options)
{
    if (options->painter == NULL || !painter_is_complete(options->painter)) {
        wlr_log(WLR_ERROR, "the painter port is missing an entry");
        return false;
    }
    if (options->services == NULL || !services_are_complete(options->services)) {
        wlr_log(WLR_ERROR, "the services port is missing an entry");
        return false;
    }
    return true;
}

static bool painter_is_complete(const compositor_painter *p)
{
    return p->paint_frame != NULL && p->measure_title != NULL && p->shadow_template != NULL &&
           p->osd.layout_menu != NULL && p->osd.paint_menu != NULL &&
           p->osd.layout_cycle != NULL && p->osd.paint_cycle != NULL;
}

static bool services_are_complete(const compositor_services *s)
{
    return s->keyboard_prefs != NULL && s->pointer_prefs != NULL && s->cursor_prefs != NULL &&
           s->lookup_icon != NULL && s->cancel_icon_lookups != NULL && s->ask_terminate != NULL;
}

static void init_lists(compositor *c)
{
    wl_list_init(&c->views);
    wl_list_init(&c->backend_destroy.link);
    wl_signal_init(&c->view_events.mapped);
    wl_signal_init(&c->view_events.changed);
    wl_signal_init(&c->view_events.unmapped);
}

static bool create_display(compositor *c)
{
    c->display = wl_display_create();
    if (c->display == NULL) {
        return false;
    }
    c->loop = wl_display_get_event_loop(c->display);
    c->timer = wl_event_loop_add_timer(c->loop, handle_timer, c);
    return c->timer != NULL;
}

static bool create_backend(compositor *c)
{
    if (!c->config.compositing.use_gl) {
        setenv("WLR_RENDERER", "pixman", 1);
    }
    c->backend = wlr_backend_autocreate(c->loop, &c->session);
    if (c->backend == NULL) {
        return false;
    }
    listener_connect(&c->backend_destroy, &c->backend->events.destroy, handle_backend_destroy);
    c->renderer = wlr_renderer_autocreate(c->backend);
    if (c->renderer == NULL) {
        return false;
    }
    c->allocator = wlr_allocator_autocreate(c->backend, c->renderer);
    return c->allocator != NULL;
}

static bool create_scene(compositor *c)
{
    c->layout = wlr_output_layout_create(c->display);
    c->scene = wlr_scene_create();
    if (c->layout == NULL || c->scene == NULL) {
        return false;
    }
    c->scene_layout = wlr_scene_attach_output_layout(c->scene, c->layout);
    c->scene->restack_xwayland_surfaces = true;
    c->scanout_allowed = c->scene->WLR_PRIVATE.direct_scanout;
    apply_scanout(c);
    c->layers = scene_layers_create(c->scene);
    c->shadows = shadow_cache_create(c->painter);
    return c->scene_layout != NULL && c->layers != NULL && c->shadows != NULL;
}

static void destroy_backend(compositor *c)
{
    listener_disconnect(&c->backend_destroy);
    if (c->timer != NULL) {
        wl_event_source_remove(c->timer);
    }
    if (c->backend != NULL) {
        wlr_backend_destroy(c->backend);
    }
    if (c->scene != NULL) {
        wlr_scene_node_destroy(&c->scene->tree.node);
    }
    if (c->layers != NULL) {
        scene_layers_destroy(c->layers);
    }
    if (c->shadows != NULL) {
        shadow_cache_destroy(c->shadows);
    }
    if (c->allocator != NULL) {
        wlr_allocator_destroy(c->allocator);
    }
    if (c->renderer != NULL) {
        wlr_renderer_destroy(c->renderer);
    }
}

static void apply_scanout(compositor *c)
{
    c->scene->WLR_PRIVATE.direct_scanout =
        c->scanout_allowed && c->config.compositing.suspend_fullscreen;
}

static void handle_backend_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    compositor *c = wl_container_of(listener, c, backend_destroy);
    listener_disconnect(&c->backend_destroy);
    c->backend = NULL;
    if (c->listener != NULL && c->listener->quit_requested != NULL) {
        c->listener->quit_requested(c->listener->self);
    }
}

static int handle_timer(void *data)
{
    compositor *c = data;
    wm_desktop_handle_timer(c->desktop);
    return 0;
}
