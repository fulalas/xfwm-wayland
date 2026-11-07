#include "keyboard_grabs.h"

#include <stdlib.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/xwayland/xwayland.h>

#include "util/listener.h"
#include "util/resource.h"
#include "xwayland-keyboard-grab-unstable-v1-protocol.h"
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
    struct wl_list link;
} keyboard_grab;

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void grab_keyboard(struct wl_client *client, struct wl_resource *manager, uint32_t id,
                          struct wl_resource *surface, struct wl_resource *seat);
static keyboard_grab *create_grab(struct wl_client *client, int version, uint32_t id);
static void handle_grab_destroyed(struct wl_resource *resource);

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
        wl_list_remove(&grab->link);
        free(grab);
    }
    wl_global_destroy(grabs->global);
    free(grabs);
}

bool keyboard_grabs_hold(const keyboard_grabs *grabs, struct wlr_surface *focused)
{
    if (wl_list_empty(&grabs->grabs) || focused == NULL) {
        return false;
    }
    return wlr_xwayland_surface_try_from_wlr_surface(focused) != NULL;
}

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    keyboard_grabs *grabs = data;
    if (client != xwayland_shell_client(grabs->shell)) {
        wl_client_post_implementation_error(client, "keyboard grabs are reserved to XWayland");
        return;
    }
    struct wl_resource *resource =
        wl_resource_create(client, &zwp_xwayland_keyboard_grab_manager_v1_interface,
                           (int) version, id);
    if (resource == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &manager_impl, grabs, NULL);
}

static void grab_keyboard(struct wl_client *client, struct wl_resource *manager, uint32_t id,
                          struct wl_resource *surface, struct wl_resource *seat)
{
    (void) surface;
    (void) seat;
    keyboard_grabs *grabs = wl_resource_get_user_data(manager);
    keyboard_grab *grab = create_grab(client, wl_resource_get_version(manager), id);
    if (grab == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
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
        wl_list_remove(&grab->link);
        free(grab);
    }
}
