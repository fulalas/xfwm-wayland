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

typedef struct window_properties {
    xcb_window_t window;
    uint32_t motif[X11_MOTIF_HINT_WORDS];
    uint32_t sides[OLD_STRUT_SIDES];
    uint32_t time_window;
    uint32_t user_time;
    uint32_t locked;
    uint32_t transient_for;
    char machine[REMOTE_HOST_MAX];
    x11_read reads[READ_COUNT];
} window_properties;

static void prepare_reads(window_properties *properties, x11_link *link);
static x11_property property_of(xcb_window_t window, xcb_atom_t atom);
static x11_read cardinals(x11_property property, uint32_t *values, uint32_t size);
static void apply_motif(xwayland_view *xv, const uint32_t *hints, uint32_t count);
static void apply_old_strut(xwayland_view *xv, const uint32_t *sides, uint32_t count);
static void apply_remote_host(xwayland_view *xv, const char *machine);
static void apply_user_time(xwayland_view *xv, x11_link *link, const x11_read *reads);
static bool is_group_transient(x11_link *link, const xwayland_view *xv, const x11_read *read);

void xwayland_view_read_properties(xwayland_view *xv)
{
    x11_link *link = &xv->server->xwayland->link;
    window_properties properties = {.window = xv->xsurface->window_id};
    const x11_read *reads = properties.reads;
    prepare_reads(&properties, link);
    x11_shape_query shape = x11_link_request_shape(link, properties.window);
    x11_link_read(link, properties.reads, READ_COUNT);
    xv->shaped = x11_link_collect_shape(link, &shape);
    apply_motif(xv, properties.motif, reads[READ_MOTIF].count);
    apply_old_strut(xv, properties.sides, reads[READ_STRUT].count);
    apply_remote_host(xv, properties.machine);
    apply_user_time(xv, link, reads);
    xv->opacity_locked = reads[READ_OPACITY_LOCK].count == 1;
    xv->transient_for_group = is_group_transient(link, xv, &reads[READ_TRANSIENT_FOR]);
}

void xwayland_view_read_motif(xwayland_view *xv)
{
    x11_link *link = &xv->server->xwayland->link;
    uint32_t motif[X11_MOTIF_HINT_WORDS];
    x11_property property =
        property_of(xv->xsurface->window_id, x11_link_atom(link, ATOM_MOTIF_WM_HINTS));
    x11_read read = cardinals(property, motif, X11_MOTIF_HINT_WORDS);
    x11_link_read(link, &read, 1);
    apply_motif(xv, motif, read.count);
}

void xwayland_view_read_old_strut(xwayland_view *xv)
{
    xwayland_shell *shell = xv->server->xwayland;
    uint32_t sides[OLD_STRUT_SIDES];
    x11_read read = {{0}, X11_READ_CARDINALS, sides, 0, 0};
    if (shell != NULL) {
        x11_property property = property_of(xv->xsurface->window_id,
                                             x11_link_atom(&shell->link, ATOM_NET_WM_STRUT));
        read = cardinals(property, sides, OLD_STRUT_SIDES);
        x11_link_read(&shell->link, &read, 1);
    }
    apply_old_strut(xv, sides, read.count);
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

static void prepare_reads(window_properties *p, x11_link *link)
{
    xcb_window_t window = p->window;
    x11_read *reads = p->reads;
    reads[READ_MOTIF] = cardinals(property_of(window, x11_link_atom(link, ATOM_MOTIF_WM_HINTS)),
                                  p->motif, X11_MOTIF_HINT_WORDS);
    reads[READ_STRUT] = cardinals(property_of(window, x11_link_atom(link, ATOM_NET_WM_STRUT)),
                                  p->sides, OLD_STRUT_SIDES);
    reads[READ_MACHINE] = (x11_read){property_of(window, XCB_ATOM_WM_CLIENT_MACHINE),
                                     X11_READ_TEXT, p->machine, sizeof(p->machine), 0};
    reads[READ_TIME_WINDOW] = cardinals(
        property_of(window, x11_link_atom(link, ATOM_NET_WM_USER_TIME_WINDOW)), &p->time_window, 1);
    reads[READ_USER_TIME] = cardinals(
        property_of(window, x11_link_atom(link, ATOM_NET_WM_USER_TIME)), &p->user_time, 1);
    reads[READ_OPACITY_LOCK] = cardinals(
        property_of(window, x11_link_atom(link, ATOM_NET_WM_WINDOW_OPACITY_LOCKED)), &p->locked, 1);
    reads[READ_TRANSIENT_FOR] =
        cardinals(property_of(window, XCB_ATOM_WM_TRANSIENT_FOR), &p->transient_for, 1);
}

static x11_property property_of(xcb_window_t window, xcb_atom_t atom)
{
    return (x11_property){window, atom, XCB_ATOM_ANY};
}

static x11_read cardinals(x11_property property, uint32_t *values, uint32_t size)
{
    return (x11_read){property, X11_READ_CARDINALS, values, size, 0};
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
    xv->has_old_strut = count == OLD_STRUT_SIDES;
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
