#include <stdlib.h>
#include <time.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>

#include "cairo_buffer.h"
#include "clock.h"
#include "find_cursor_paint.h"
#include "listener.h"
#include "osd_parts.h"
#include "output.h"
#include "scene_buffers.h"
#include "seat.h"

static const double DEFAULT_SCALE = 1.0;

struct osd_find_cursor {
    compositor *server;
    struct wlr_scene_buffer *node;
    struct timespec start;
    struct wl_listener frame_done;
};

static void handle_frame_done(struct wl_listener *listener, void *data);
static void draw(osd_find_cursor *find, uint32_t elapsed_ms);
static void stop(osd_find_cursor *find);
static double scale_at(const compositor *server, wm_point point);
static uint32_t elapsed_ms(const struct timespec *from, const struct timespec *to);

osd_find_cursor *osd_find_cursor_create(compositor *server, struct wlr_scene_tree *parent)
{
    osd_find_cursor *find = calloc(1, sizeof(*find));
    if (find == NULL) {
        return NULL;
    }
    find->server = server;
    find->node = wlr_scene_buffer_create(parent, NULL);
    if (find->node == NULL) {
        free(find);
        return NULL;
    }
    find->node->point_accepts_input = scene_buffer_reject_input;
    wlr_scene_node_set_enabled(&find->node->node, false);
    wl_list_init(&find->frame_done.link);
    return find;
}

void osd_find_cursor_destroy(osd_find_cursor *find)
{
    listener_disconnect(&find->frame_done);
    wlr_scene_node_destroy(&find->node->node);
    free(find);
}

void osd_find_cursor_start(osd_find_cursor *find)
{
    clock_gettime(CLOCK_MONOTONIC, &find->start);
    listener_disconnect(&find->frame_done);
    listener_connect(&find->frame_done, &find->node->events.frame_done, handle_frame_done);
    draw(find, 0);
}

static void handle_frame_done(struct wl_listener *listener, void *data)
{
    osd_find_cursor *find = wl_container_of(listener, find, frame_done);
    const struct wlr_scene_frame_done_event *event = data;
    uint32_t elapsed = elapsed_ms(&find->start, &event->when);
    if (find_cursor_is_running(elapsed)) {
        draw(find, elapsed);
    } else {
        stop(find);
    }
}

static void draw(osd_find_cursor *find, uint32_t elapsed)
{
    find_cursor_frame frame = find_cursor_frame_at(elapsed);
    wm_point pointer = seat_pointer_position(find->server->seat);
    int half = find_cursor_half_size(&frame);
    cairo_surface_t *surface = find_cursor_paint(&frame, scale_at(find->server, pointer));
    if (!cairo_buffer_attach_sized(find->node, surface, (wm_size){2 * half, 2 * half})) {
        stop(find);
        return;
    }
    wlr_scene_node_set_position(&find->node->node, pointer.x - half, pointer.y - half);
    wlr_scene_node_set_enabled(&find->node->node, true);
}

static void stop(osd_find_cursor *find)
{
    listener_disconnect(&find->frame_done);
    wlr_scene_node_set_enabled(&find->node->node, false);
    wlr_scene_buffer_set_buffer(find->node, NULL);
}

static double scale_at(const compositor *server, wm_point point)
{
    output *o = outputs_at_point(server->outputs, point);
    return o != NULL ? output_wlr(o)->scale : DEFAULT_SCALE;
}

static uint32_t elapsed_ms(const struct timespec *from, const struct timespec *to)
{
    int64_t ms = ((int64_t) to->tv_sec - from->tv_sec) * CLOCK_MS_PER_SECOND +
                 ((int64_t) to->tv_nsec - from->tv_nsec) / CLOCK_NS_PER_MS;
    return ms > 0 ? (uint32_t) ms : 0;
}
