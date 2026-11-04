#include <stdio.h>
#include <string.h>
#include <wlr/util/log.h>
#include <wlr/xwayland/server.h>
#include <wlr/xwayland/xwayland.h>

#include "clock.h"
#include "listener.h"
#include "xwayland_shell_private.h"

enum {
    CHECK_DELAY_MS = 100,
    MS_PER_SECOND = 1000,
    NS_PER_MS = 1000000,
};

static void handle_server_start(struct wl_listener *listener, void *data);
static void handle_client_destroy(struct wl_listener *listener, void *data);
static int handle_timer(void *data);
static void check_server(xwayland_shell *shell);
static void restart_server(xwayland_shell *shell);
static bool server_is_gone(const struct wlr_xwayland *wlr);
static void report_display(const xwayland_shell *shell, const char *previous);

void xwayland_restart_init(xwayland_shell *shell)
{
    restart_state *restart = &shell->restart;
    wl_list_init(&restart->server_start.link);
    wl_list_init(&restart->client_destroy.link);
    restart->timer = wl_event_loop_add_timer(shell->server->loop, handle_timer, shell);
}

void xwayland_restart_watch(xwayland_shell *shell)
{
    listener_connect(&shell->restart.server_start, &shell->wlr->server->events.start,
                     handle_server_start);
}

void xwayland_restart_unwatch(xwayland_shell *shell)
{
    listener_disconnect(&shell->restart.server_start);
    listener_disconnect(&shell->restart.client_destroy);
}

void xwayland_restart_finish(xwayland_shell *shell)
{
    if (shell->restart.timer != NULL) {
        wl_event_source_remove(shell->restart.timer);
        shell->restart.timer = NULL;
    }
}

static void handle_server_start(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_shell *shell = wl_container_of(listener, shell, restart.server_start);
    struct wl_client *client = shell->wlr->server->client;
    listener_disconnect(&shell->restart.client_destroy);
    if (client != NULL) {
        shell->restart.client_destroy.notify = handle_client_destroy;
        wl_client_add_destroy_listener(client, &shell->restart.client_destroy);
    }
}

static void handle_client_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    xwayland_shell *shell = wl_container_of(listener, shell, restart.client_destroy);
    listener_disconnect(&shell->restart.client_destroy);
    shell->restart.phase = RESTART_CHECKING;
    wl_event_source_timer_update(shell->restart.timer, CHECK_DELAY_MS);
}

static int handle_timer(void *data)
{
    xwayland_shell *shell = data;
    restart_phase phase = shell->restart.phase;
    shell->restart.phase = RESTART_IDLE;
    if (phase == RESTART_CHECKING) {
        check_server(shell);
    } else if (phase == RESTART_WAITING) {
        restart_server(shell);
    }
    return 0;
}

static void check_server(xwayland_shell *shell)
{
    if (!server_is_gone(shell->wlr)) {
        return;
    }
    restart_policy_record_crash(&shell->restart.policy, clock_now_ms());
    restart_decision decision = restart_policy_next(&shell->restart.policy);
    if (!decision.restart) {
        wlr_log(WLR_ERROR, "XWayland keeps crashing, X11 applications stay unavailable");
        return;
    }
    wlr_log(WLR_ERROR, "XWayland stopped, restarting it in %u ms", decision.delay_ms);
    shell->restart.phase = RESTART_WAITING;
    wl_event_source_timer_update(shell->restart.timer, (int) decision.delay_ms);
}

static void restart_server(xwayland_shell *shell)
{
    char previous[DISPLAY_NAME_MAX];
    snprintf(previous, sizeof(previous), "%s", shell->display_name);
    xwayland_shell_stop(shell);
    if (xwayland_shell_start(shell)) {
        report_display(shell, previous);
    }
}

static bool server_is_gone(const struct wlr_xwayland *wlr)
{
    const struct wlr_xwayland_server *server = wlr != NULL ? wlr->server : NULL;
    if (server == NULL) {
        return false;
    }
    return server->client == NULL && server->x_fd_read_event[0] == NULL &&
           server->idle_source == NULL;
}

static void report_display(const xwayland_shell *shell, const char *previous)
{
    if (strcmp(previous, shell->display_name) == 0) {
        return;
    }
    wlr_log(WLR_ERROR, "XWayland moved from DISPLAY %s to %s", previous, shell->display_name);
    const compositor_listener *listener = shell->server->listener;
    if (listener != NULL && listener->x11_display_changed != NULL) {
        listener->x11_display_changed(listener->self);
    }
}
