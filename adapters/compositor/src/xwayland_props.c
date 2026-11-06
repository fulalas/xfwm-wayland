#include <libintl.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <wlr/xwayland/xwayland.h>

#include "x11_motif.h"
#include "xwayland_shell_private.h"
#include "xwayland_view_private.h"

enum {
    READ_MOTIF,
    READ_STRUT,
    READ_MACHINE,
    READ_TIME_WINDOW,
    READ_USER_TIME,
    READ_OPACITY_LOCK,
    READ_TRANSIENT_FOR,
    READ_COUNT,
};

static const char XFWM4_DOMAIN[] = "xfwm4";

static x11_read cardinals(xcb_window_t window, xcb_atom_t atom, uint32_t *values,
                          uint32_t size);
static void apply_motif(xwayland_view *xv, const uint32_t *hints, uint32_t count);
static void apply_old_strut(xwayland_view *xv, const uint32_t *sides, uint32_t count);
static void apply_remote_host(xwayland_view *xv, const char *machine);
static void apply_user_time(xwayland_view *xv, x11_link *link, const x11_read *reads);
static bool is_group_transient(x11_link *link, const xwayland_view *xv, const x11_read *read);

void xwayland_view_read_properties(xwayland_view *xv)
{
    xwayland_shell *shell = xv->server->xwayland;
    if (shell == NULL) {
        return;
    }
    x11_link *link = &shell->link;
    xcb_window_t window = xv->xsurface->window_id;
    uint32_t motif[X11_MOTIF_HINT_WORDS];
    uint32_t sides[OLD_STRUT_SIDES];
    uint32_t time_window;
    uint32_t user_time;
    uint32_t locked;
    uint32_t transient_for;
    char machine[REMOTE_HOST_MAX] = "";
    x11_read reads[READ_COUNT] = {
        [READ_MOTIF] = cardinals(window, x11_link_atom(link, ATOM_MOTIF_WM_HINTS), motif,
                                 X11_MOTIF_HINT_WORDS),
        [READ_STRUT] = cardinals(window, x11_link_atom(link, ATOM_NET_WM_STRUT), sides,
                                 OLD_STRUT_SIDES),
        [READ_MACHINE] = {{window, XCB_ATOM_WM_CLIENT_MACHINE, XCB_ATOM_ANY}, X11_READ_TEXT,
                          machine, sizeof(machine), 0},
        [READ_TIME_WINDOW] = cardinals(window, x11_link_atom(link, ATOM_NET_WM_USER_TIME_WINDOW),
                                       &time_window, 1),
        [READ_USER_TIME] = cardinals(window, x11_link_atom(link, ATOM_NET_WM_USER_TIME),
                                     &user_time, 1),
        [READ_OPACITY_LOCK] = cardinals(
            window, x11_link_atom(link, ATOM_NET_WM_WINDOW_OPACITY_LOCKED), &locked, 1),
        [READ_TRANSIENT_FOR] = cardinals(window, XCB_ATOM_WM_TRANSIENT_FOR, &transient_for, 1),
    };
    x11_link_read(link, reads, READ_COUNT);
    xv->shaped = x11_link_is_shaped(link, window);
    apply_motif(xv, motif, reads[READ_MOTIF].count);
    apply_old_strut(xv, sides, reads[READ_STRUT].count);
    apply_remote_host(xv, machine);
    apply_user_time(xv, link, reads);
    xv->opacity_locked = reads[READ_OPACITY_LOCK].count == 1;
    xv->transient_for_group = is_group_transient(link, xv, &reads[READ_TRANSIENT_FOR]);
}

void xwayland_view_read_motif(xwayland_view *xv)
{
    xwayland_shell *shell = xv->server->xwayland;
    uint32_t motif[X11_MOTIF_HINT_WORDS];
    x11_read read = {{0}, X11_READ_CARDINALS, motif, 0, 0};
    if (shell != NULL) {
        read = cardinals(xv->xsurface->window_id,
                         x11_link_atom(&shell->link, ATOM_MOTIF_WM_HINTS), motif,
                         X11_MOTIF_HINT_WORDS);
        x11_link_read(&shell->link, &read, 1);
    }
    apply_motif(xv, motif, read.count);
}

char *xwayland_view_remote_title(const xwayland_view *xv)
{
    if (xv->remote_host[0] == '\0') {
        return NULL;
    }
    const char *title = xv->xsurface->title != NULL ? xv->xsurface->title : "";
    char *text = NULL;
    if (asprintf(&text, dgettext(XFWM4_DOMAIN, "%s (on %s)"), title, xv->remote_host) < 0) {
        return NULL;
    }
    return text;
}

static x11_read cardinals(xcb_window_t window, xcb_atom_t atom, uint32_t *values,
                          uint32_t size)
{
    return (x11_read){{window, atom, XCB_ATOM_ANY}, X11_READ_CARDINALS, values, size, 0};
}

static void apply_motif(xwayland_view *xv, const uint32_t *hints, uint32_t count)
{
    xv->motif_capabilities = x11_motif_capabilities(hints, count);
    if (xv->shaped) {
        xv->motif_capabilities &= ~(uint32_t) WM_CAP_DECORATED;
    }
}

static void apply_old_strut(xwayland_view *xv, const uint32_t *sides, uint32_t count)
{
    xv->has_old_strut = xv->xsurface->strut_partial == NULL && count == OLD_STRUT_SIDES;
    if (xv->has_old_strut) {
        xv->old_strut = (xcb_ewmh_wm_strut_partial_t){
            .left = sides[0], .right = sides[1], .top = sides[2], .bottom = sides[3],
            .left_end_y = INT32_MAX, .right_end_y = INT32_MAX,
            .top_end_x = INT32_MAX, .bottom_end_x = INT32_MAX,
        };
    }
}

static void apply_remote_host(xwayland_view *xv, const char *machine)
{
    char local[REMOTE_HOST_MAX] = "";
    bool remote = machine[0] != '\0' && gethostname(local, sizeof(local) - 1) == 0 &&
                  strcasecmp(machine, local) != 0;
    snprintf(xv->remote_host, sizeof(xv->remote_host), "%s", remote ? machine : "");
}

static void apply_user_time(xwayland_view *xv, x11_link *link, const x11_read *reads)
{
    x11_read time = reads[READ_USER_TIME];
    if (reads[READ_TIME_WINDOW].count == 1) {
        time.property.window = *(const uint32_t *) reads[READ_TIME_WINDOW].data;
        x11_link_read(link, &time, 1);
    }
    xv->has_user_time = time.count == 1;
    xv->user_time = xv->has_user_time ? *(const uint32_t *) time.data : 0;
}

static bool is_group_transient(x11_link *link, const xwayland_view *xv, const x11_read *read)
{
    xcb_connection_t *connection = x11_link_query_connection(link);
    const xcb_icccm_wm_hints_t *hints = xv->xsurface->hints;
    bool grouped = hints != NULL && (hints->flags & XCB_ICCCM_WM_HINT_WINDOW_GROUP) != 0;
    if (read->count != 1 || !grouped || connection == NULL) {
        return false;
    }
    uint32_t transient_for = *(const uint32_t *) read->data;
    return transient_for == XCB_NONE || transient_for == x11_link_root(connection);
}
