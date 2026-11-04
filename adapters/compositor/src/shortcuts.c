#include <stdlib.h>
#include <wlr/types/wlr_keyboard_shortcuts_inhibit_v1.h>
#include <wlr/types/wlr_seat.h>

#include "listener.h"
#include "seat_private.h"

struct shortcuts {
    seat *seat;
    struct wlr_keyboard_shortcuts_inhibit_manager_v1 *manager;
    struct wl_listener new_inhibitor;
};

static void handle_new_inhibitor(struct wl_listener *listener, void *data);
static void update_inhibitor(struct wlr_keyboard_shortcuts_inhibitor_v1 *inhibitor,
                             const struct wlr_surface *focused);

shortcuts *shortcuts_create(seat *s)
{
    shortcuts *sc = calloc(1, sizeof(*sc));
    if (sc == NULL) {
        return NULL;
    }
    sc->seat = s;
    sc->manager = s->server->globals.shortcuts_inhibit;
    wl_list_init(&sc->new_inhibitor.link);
    if (sc->manager != NULL) {
        listener_connect(&sc->new_inhibitor, &sc->manager->events.new_inhibitor,
                         handle_new_inhibitor);
    }
    return sc;
}

void shortcuts_destroy(shortcuts *sc)
{
    listener_disconnect(&sc->new_inhibitor);
    free(sc);
}

void shortcuts_focus_changed(shortcuts *sc, struct wlr_surface *surface)
{
    if (sc->manager == NULL) {
        return;
    }
    struct wlr_keyboard_shortcuts_inhibitor_v1 *inhibitor;
    wl_list_for_each(inhibitor, &sc->manager->inhibitors, link) {
        if (inhibitor->seat == sc->seat->wlr_seat) {
            update_inhibitor(inhibitor, surface);
        }
    }
}

bool shortcuts_inhibited(const shortcuts *sc, const struct wlr_surface *surface)
{
    if (sc->manager == NULL || surface == NULL) {
        return false;
    }
    struct wlr_keyboard_shortcuts_inhibitor_v1 *inhibitor;
    wl_list_for_each(inhibitor, &sc->manager->inhibitors, link) {
        if (inhibitor->active && inhibitor->surface == surface &&
            inhibitor->seat == sc->seat->wlr_seat) {
            return true;
        }
    }
    return false;
}

static void handle_new_inhibitor(struct wl_listener *listener, void *data)
{
    shortcuts *sc = wl_container_of(listener, sc, new_inhibitor);
    update_inhibitor(data, sc->seat->focus.current);
}

static void update_inhibitor(struct wlr_keyboard_shortcuts_inhibitor_v1 *inhibitor,
                             const struct wlr_surface *focused)
{
    bool wanted = inhibitor->surface == focused;
    if (wanted && !inhibitor->active) {
        wlr_keyboard_shortcuts_inhibitor_v1_activate(inhibitor);
    } else if (!wanted && inhibitor->active) {
        wlr_keyboard_shortcuts_inhibitor_v1_deactivate(inhibitor);
    }
}
