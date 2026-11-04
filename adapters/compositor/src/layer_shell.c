#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/util/log.h>

#include "layer_shell_private.h"
#include "listener.h"
#include "seat.h"

enum {
    LAYER_SHELL_VERSION = 4,
    SHELL_LAYER_COUNT = 4,
};

static const uint32_t layers_top_down[SHELL_LAYER_COUNT] = {
    ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
    ZWLR_LAYER_SHELL_V1_LAYER_TOP,
    ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM,
    ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND,
};

typedef struct arrange_pass {
    output *output;
    uint32_t layer;
    bool exclusive;
} arrange_pass;

static void handle_new_surface(struct wl_listener *listener, void *data);
static output *output_for_new_surface(const layer_shell *shell);
static wm_rect arrange_layer(layer_shell *shell, const arrange_pass *pass, layer_area area);
static bool takes_exclusive_focus(const layer_surface *surface);

layer_shell *layer_shell_create(compositor *server)
{
    layer_shell *shell = calloc(1, sizeof(*shell));
    if (shell == NULL) {
        return NULL;
    }
    shell->server = server;
    wl_list_init(&shell->surfaces);
    shell->wlr_shell = wlr_layer_shell_v1_create(server->display, LAYER_SHELL_VERSION);
    if (shell->wlr_shell == NULL) {
        wlr_log(WLR_ERROR, "cannot create the layer-shell global");
        free(shell);
        return NULL;
    }
    listener_connect(&shell->new_surface, &shell->wlr_shell->events.new_surface,
                     handle_new_surface);
    return shell;
}

void layer_shell_destroy(layer_shell *shell)
{
    listener_disconnect(&shell->new_surface);
    free(shell);
}

wm_rect layer_shell_arrange(layer_shell *shell, output *o)
{
    layer_area area = {output_full(o), output_full(o)};
    for (int exclusive = 1; exclusive >= 0; exclusive--) {
        for (int i = 0; i < SHELL_LAYER_COUNT; i++) {
            arrange_pass pass = {o, layers_top_down[i], exclusive == 1};
            area.usable = arrange_layer(shell, &pass, area);
        }
    }
    return area.usable;
}

struct wlr_surface *layer_shell_exclusive_focus(const layer_shell *shell)
{
    struct wlr_surface *top = NULL;
    layer_surface *surface;
    wl_list_for_each(surface, &shell->surfaces, link) {
        if (!takes_exclusive_focus(surface)) {
            continue;
        }
        if (top == NULL || surface->wlr->current.layer == ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY) {
            top = surface->wlr->surface;
        }
    }
    return top;
}

void layer_shell_output_destroyed(layer_shell *shell, output *o)
{
    layer_surface *surface;
    layer_surface *tmp;
    wl_list_for_each_safe(surface, tmp, &shell->surfaces, link) {
        if (surface->output == o) {
            surface->output = NULL;
            surface->wlr->output = NULL;
            wlr_layer_surface_v1_destroy(surface->wlr);
        }
    }
}

bool layer_surface_accepts_click_focus(const layer_surface *surface)
{
    return surface->wlr->current.keyboard_interactive !=
           ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE;
}

struct wlr_surface *layer_surface_wlr_surface(const layer_surface *surface)
{
    return surface->wlr->surface;
}

static void handle_new_surface(struct wl_listener *listener, void *data)
{
    layer_shell *shell = wl_container_of(listener, shell, new_surface);
    struct wlr_layer_surface_v1 *wlr = data;
    if (wlr->output == NULL) {
        output *o = output_for_new_surface(shell);
        if (o == NULL) {
            wlr_layer_surface_v1_destroy(wlr);
            return;
        }
        wlr->output = output_wlr(o);
    }
    if (layer_surface_create(shell, wlr) == NULL) {
        wlr_log(WLR_ERROR, "cannot create layer surface %s", wlr->namespace);
        wlr_layer_surface_v1_destroy(wlr);
    }
}

static output *output_for_new_surface(const layer_shell *shell)
{
    compositor *server = shell->server;
    return outputs_near_point(server->outputs, seat_pointer_position(server->seat));
}

static wm_rect arrange_layer(layer_shell *shell, const arrange_pass *pass, layer_area area)
{
    layer_surface *surface;
    wl_list_for_each(surface, &shell->surfaces, link) {
        const struct wlr_layer_surface_v1_state *state = &surface->wlr->current;
        if (surface->output != pass->output || !surface->wlr->initialized ||
            state->layer != pass->layer || (state->exclusive_zone > 0) != pass->exclusive) {
            continue;
        }
        area.usable = layer_surface_configure(surface, &area);
    }
    return area.usable;
}

static bool takes_exclusive_focus(const layer_surface *surface)
{
    const struct wlr_layer_surface_v1_state *state = &surface->wlr->current;
    return surface->wlr->surface->mapped &&
           state->keyboard_interactive == ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE &&
           (state->layer == ZWLR_LAYER_SHELL_V1_LAYER_TOP ||
            state->layer == ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY);
}
