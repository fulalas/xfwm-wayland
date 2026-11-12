#include "keyboard_grabs.h"

#include <stdlib.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/xwayland/xwayland.h>

#include "util/listener.h"
#include "util/resource.h"
#include "xwayland-keyboard-grab-unstable-v1-protocol.h"
#include "xwayland/x11_link.h"
#include "xwayland/xwayland.h"

enum {
    GRAB_MANAGER_VERSION = 1,
};

struct keyboard_grabs {
    xwayland_shell *shell;
    struct wl_global *global;
    struct wl_list grabs;
};

typedef struct keyboard_grab {
    struct wl_resource *resource;
    struct wlr_surface *surface;
    struct wl_list link;
    struct wl_listener surface_destroy;
} keyboard_grab;

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void grab_keyboard(struct wl_client *client, struct wl_resource *manager, uint32_t id,
                          struct wl_resource *surface, struct wl_resource *seat);
static keyboard_grab *create_grab(struct wl_client *client, int version, uint32_t id);
static void handle_grab_destroyed(struct wl_resource *resource);
static void handle_surface_destroy(struct wl_listener *listener, void *data);
static void free_grab(keyboard_grab *grab);
static bool same_x11_client(const keyboard_grabs *grabs, struct wlr_surface *surface,
                            const struct wlr_xwayland_surface *target);

static const struct zwp_xwayland_keyboard_grab_manager_v1_interface manager_impl = {
    .destroy = resource_handle_destroy,
    .grab_keyboard = grab_keyboard,
};

static const struct zwp_xwayland_keyboard_grab_v1_interface grab_impl = {
    .destroy = resource_handle_destroy,
};

keyboard_grabs *keyboard_grabs_create(struct wl_display *display, xwayland_shell *shell)
{
    keyboard_grabs *grabs = calloc(1, sizeof(*grabs));
    if (grabs == NULL) {
        return NULL;
    }
    grabs->shell = shell;
    wl_list_init(&grabs->grabs);
    grabs->global = wl_global_create(display, &zwp_xwayland_keyboard_grab_manager_v1_interface,
                                     GRAB_MANAGER_VERSION, grabs, bind_manager);
    if (grabs->global == NULL) {
        free(grabs);
        return NULL;
    }
    return grabs;
}

void keyboard_grabs_destroy(keyboard_grabs *grabs)
{
    keyboard_grab *grab;
    keyboard_grab *next;
    wl_list_for_each_safe(grab, next, &grabs->grabs, link) {
        wl_resource_set_user_data(grab->resource, NULL);
        wl_resource_set_destructor(grab->resource, NULL);
        free_grab(grab);
    }
    wl_global_destroy(grabs->global);
    free(grabs);
}

bool keyboard_grabs_hold(const keyboard_grabs *grabs, struct wlr_surface *focused)
{
    if (wl_list_empty(&grabs->grabs) || focused == NULL) {
        return false;
    }
    struct wlr_xwayland_surface *target = wlr_xwayland_surface_try_from_wlr_surface(focused);
    if (target == NULL) {
        return false;
    }
    const keyboard_grab *grab;
    wl_list_for_each(grab, &grabs->grabs, link) {
        if (grab->surface != NULL && same_x11_client(grabs, grab->surface, target)) {
            return true;
        }
    }
    return false;
}

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    keyboard_grabs *grabs = data;
    if (client != xwayland_shell_client(grabs->shell)) {
        wl_client_post_implementation_error(client, "keyboard grabs are reserved to XWayland");
        return;
    }
    resource_bind(client, &zwp_xwayland_keyboard_grab_manager_v1_interface, version, id,
                  &manager_impl, grabs, NULL);
}

static void grab_keyboard(struct wl_client *client, struct wl_resource *manager, uint32_t id,
                          struct wl_resource *surface, struct wl_resource *seat)
{
    (void) seat;
    keyboard_grabs *grabs = wl_resource_get_user_data(manager);
    keyboard_grab *grab = create_grab(client, wl_resource_get_version(manager), id);
    if (grab == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    grab->surface = wlr_surface_from_resource(surface);
    listener_connect(&grab->surface_destroy, &grab->surface->events.destroy,
                     handle_surface_destroy);
    wl_list_insert(&grabs->grabs, &grab->link);
}

static keyboard_grab *create_grab(struct wl_client *client, int version, uint32_t id)
{
    keyboard_grab *grab = calloc(1, sizeof(*grab));
    if (grab == NULL) {
        return NULL;
    }
    grab->resource =
        wl_resource_create(client, &zwp_xwayland_keyboard_grab_v1_interface, version, id);
    if (grab->resource == NULL) {
        free(grab);
        return NULL;
    }
    wl_resource_set_implementation(grab->resource, &grab_impl, grab, handle_grab_destroyed);
    return grab;
}

static void handle_grab_destroyed(struct wl_resource *resource)
{
    keyboard_grab *grab = wl_resource_get_user_data(resource);
    if (grab != NULL) {
        free_grab(grab);
    }
}

static void handle_surface_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    keyboard_grab *grab = wl_container_of(listener, grab, surface_destroy);
    listener_disconnect(&grab->surface_destroy);
    grab->surface = NULL;
}

static void free_grab(keyboard_grab *grab)
{
    listener_disconnect(&grab->surface_destroy);
    wl_list_remove(&grab->link);
    free(grab);
}

static bool same_x11_client(const keyboard_grabs *grabs, struct wlr_surface *surface,
                            const struct wlr_xwayland_surface *target)
{
    struct wlr_xwayland_surface *grabbing = wlr_xwayland_surface_try_from_wlr_surface(surface);
    xcb_connection_t *connection = x11_link_connection(xwayland_shell_link(grabs->shell));
    if (grabbing == NULL || connection == NULL) {
        return false;
    }
    uint32_t client_bits = ~xcb_get_setup(connection)->resource_id_mask;
    return (grabbing->window_id & client_bits) == (target->window_id & client_bits);
}
