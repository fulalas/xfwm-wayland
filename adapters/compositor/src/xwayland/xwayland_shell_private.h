#ifndef COMPOSITOR_XWAYLAND_SHELL_PRIVATE_H
#define COMPOSITOR_XWAYLAND_SHELL_PRIVATE_H

#include <stdbool.h>
#include <wayland-server-core.h>
#include <xcb/xcb.h>

#include "server/restart_policy.h"
#include "x11_link.h"
#include "xwayland.h"

enum {
    DISPLAY_NAME_MAX = 32,
};

typedef struct xwayland_ewmh xwayland_ewmh;
typedef struct xwayland_xsettings xwayland_xsettings;
typedef struct keyboard_grabs keyboard_grabs;

typedef enum restart_phase {
    RESTART_IDLE,
    RESTART_CHECKING,
    RESTART_WAITING,
} restart_phase;

typedef struct restart_state {
    restart_policy policy;
    restart_phase phase;
    struct wl_event_source *timer;
    struct wl_listener server_start;
    struct wl_listener client_destroy;
} restart_state;

enum {
    STARTUP_IDS_MAX = 8,
};

typedef struct startup_id_entry {
    pid_t pid;
    char id[WM_LAUNCH_ID_MAX];
} startup_id_entry;

struct xwayland_shell {
    compositor *server;
    struct wlr_xwayland *wlr;
    struct wl_list views;
    x11_link link;
    xwayland_ewmh *ewmh;
    xwayland_xsettings *xsettings;
    keyboard_grabs *grabs;
    restart_state restart;
    char display_name[DISPLAY_NAME_MAX];
    struct wl_listener ready;
    struct wl_listener new_surface;
    struct wl_listener startup_done;
    startup_id_entry startup_ids[STARTUP_IDS_MAX];
    size_t next_startup_id;
};

bool xwayland_shell_start(xwayland_shell *shell);
void xwayland_shell_stop(xwayland_shell *shell);
bool xwayland_messages_handle(xwayland_shell *shell, xcb_generic_event_t *event);
void xwayland_restart_init(xwayland_shell *shell);
void xwayland_restart_watch(xwayland_shell *shell);
void xwayland_restart_unwatch(xwayland_shell *shell);
void xwayland_restart_finish(xwayland_shell *shell);
void xwayland_shell_note_startup_id(xwayland_shell *shell, pid_t pid, const char *id);
const char *xwayland_shell_take_startup_id(xwayland_shell *shell, pid_t pid);

#endif
