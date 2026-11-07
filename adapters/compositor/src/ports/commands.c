#include <stdlib.h>

#include "util/listener.h"
#include "input/seat.h"
#include "server/server.h"
#include "shell/view.h"

typedef struct terminate_ticket {
    compositor *server;
    view *view;
    struct wl_listener unmapped;
} terminate_ticket;

static void handle_terminate_answer(void *context, bool terminate);
static void handle_ticket_unmapped(struct wl_listener *listener, void *data);

void compositor_configure_window(compositor *c, void *window, const wm_rect *client)
{
    (void) c;
    view_configure(window, client);
}

void compositor_close_window(compositor *c, void *window)
{
    (void) c;
    view_close(window);
}

void compositor_ask_terminate(compositor *c, void *window)
{
    const compositor_services *services = c->services;
    terminate_ticket *ticket = calloc(1, sizeof(*ticket));
    if (ticket == NULL) {
        return;
    }
    ticket->server = c;
    ticket->view = window;
    listener_connect(&ticket->unmapped, &c->view_events.unmapped, handle_ticket_unmapped);
    terminate_request request = {
        .title = view_title(window),
        .done = handle_terminate_answer,
        .context = ticket,
    };
    services->ask_terminate(services->self, &request);
}

void compositor_warp_pointer(compositor *c, wm_point point)
{
    seat_warp(c->seat, point);
}

void compositor_schedule_timer(compositor *c, uint32_t delay_ms)
{
    if (c->timer != NULL) {
        wl_event_source_timer_update(c->timer, delay_ms > 0 ? (int) delay_ms : 1);
    }
}

static void handle_terminate_answer(void *context, bool terminate)
{
    terminate_ticket *ticket = context;
    if (terminate) {
        view_kill(ticket->view);
    }
    listener_disconnect(&ticket->unmapped);
    free(ticket);
}

static void handle_ticket_unmapped(struct wl_listener *listener, void *data)
{
    terminate_ticket *ticket = wl_container_of(listener, ticket, unmapped);
    if (ticket->view != data) {
        return;
    }
    const compositor_services *services = ticket->server->services;
    listener_disconnect(&ticket->unmapped);
    services->cancel_terminate(services->self, ticket);
    free(ticket);
}
