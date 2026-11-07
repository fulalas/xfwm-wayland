#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_xdg_toplevel_icon_v1.h>

#include "icon_convert.h"
#include "icon_pixels.h"
#include "util/listener.h"
#include "toplevel_icons.h"
#include "shell/xdg_view.h"

enum {
    ICON_WANTED_SIZE = 64,
    ICON_BUFFERS_MAX = 16,
};

struct toplevel_icons {
    compositor *server;
    struct wl_listener set_icon;
};

static void handle_set_icon(struct wl_listener *listener, void *data);
static struct wlr_buffer *best_buffer(const struct wlr_xdg_toplevel_icon_v1 *icon);
static void apply_icon(xdg_view *view, const struct wlr_xdg_toplevel_icon_v1 *icon);

toplevel_icons *toplevel_icons_create(compositor *server)
{
    toplevel_icons *icons = calloc(1, sizeof(*icons));
    if (icons == NULL) {
        return NULL;
    }
    icons->server = server;
    wl_list_init(&icons->set_icon.link);
    struct wlr_xdg_toplevel_icon_manager_v1 *manager = server->globals.toplevel_icon;
    if (manager != NULL) {
        int preferred_sizes[] = {32, 48, 64};
        wlr_xdg_toplevel_icon_manager_v1_set_sizes(
            manager, preferred_sizes, sizeof(preferred_sizes) / sizeof(preferred_sizes[0]));
        listener_connect(&icons->set_icon, &manager->events.set_icon, handle_set_icon);
    }
    return icons;
}

void toplevel_icons_destroy(toplevel_icons *icons)
{
    listener_disconnect(&icons->set_icon);
    free(icons);
}

static void handle_set_icon(struct wl_listener *listener, void *data)
{
    (void) listener;
    const struct wlr_xdg_toplevel_icon_manager_v1_set_icon_event *event = data;
    xdg_view *xv = xdg_view_from_toplevel(event->toplevel);
    if (xv != NULL) {
        apply_icon(xv, event->icon);
    }
}

static void apply_icon(xdg_view *xv, const struct wlr_xdg_toplevel_icon_v1 *icon)
{
    app_icon wanted = {xdg_view_app_id(xv), NULL, NULL};
    if (icon != NULL) {
        struct wlr_buffer *buffer = best_buffer(icon);
        wanted.name = icon->name;
        wanted.pixels = buffer != NULL ? icon_convert_buffer(buffer) : NULL;
    }
    icon_slot_set_app(xdg_view_icon_slot(xv), &wanted);
    if (wanted.pixels != NULL) {
        cairo_surface_destroy(wanted.pixels);
    }
}

static struct wlr_buffer *best_buffer(const struct wlr_xdg_toplevel_icon_v1 *icon)
{
    wm_size sizes[ICON_BUFFERS_MAX];
    struct wlr_buffer *buffers[ICON_BUFFERS_MAX];
    size_t count = 0;
    struct wlr_xdg_toplevel_icon_v1_buffer *entry;
    wl_list_for_each(entry, &icon->buffers, link) {
        if (count == ICON_BUFFERS_MAX) {
            break;
        }
        int scale = entry->scale > 0 ? entry->scale : 1;
        sizes[count] = (wm_size){entry->buffer->width / scale, entry->buffer->height / scale};
        buffers[count++] = entry->buffer;
    }
    if (count == 0) {
        return NULL;
    }
    return buffers[icon_pixels_best_index(sizes, count, ICON_WANTED_SIZE)];
}
