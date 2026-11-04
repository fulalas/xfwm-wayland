#include <stdlib.h>

#include "seat.h"
#include "server.h"
#include "view.h"

typedef struct terminate_ticket {
    pid_t pid;
} terminate_ticket;

static void handle_terminate_answer(void *context, bool terminate);

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

void compositor_kill_window(compositor *c, void *window)
{
    (void) c;
    view_kill(window);
}

void compositor_ask_terminate(compositor *c, void *window)
{
    const compositor_services *services = c->services;
    terminate_ticket *ticket = calloc(1, sizeof(*ticket));
    if (ticket == NULL) {
        return;
    }
    ticket->pid = view_pid(window);
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
        kill_client_pid(ticket->pid);
    }
    free(ticket);
}
