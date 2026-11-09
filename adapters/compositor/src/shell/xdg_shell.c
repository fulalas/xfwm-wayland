#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_dialog_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "util/listener.h"
#include "popup.h"
#include "xdg_shell.h"
#include "xdg_view.h"

enum {
    XDG_SHELL_VERSION = 6,
};

static const char toplevel_class[] = "xdg_toplevel";

struct xdg_shell {
    compositor *server;
    struct wlr_xdg_shell *wlr_shell;
    struct wl_listener new_toplevel;
    struct wl_listener new_popup;
    struct wl_listener new_decoration;
    struct wl_listener new_kde_decoration;
    struct wl_listener new_dialog;
    struct wl_protocol_logger *parent_watch;
};

typedef struct dialog_watch {
    struct wlr_xdg_dialog_v1 *dialog;
    struct wl_listener set_modal;
    struct wl_listener destroy;
} dialog_watch;

static void handle_new_toplevel(struct wl_listener *listener, void *data);
static void handle_new_popup(struct wl_listener *listener, void *data);
static void handle_new_decoration(struct wl_listener *listener, void *data);
static void handle_new_kde_decoration(struct wl_listener *listener, void *data);
static void handle_new_dialog(struct wl_listener *listener, void *data);
static void handle_dialog_modal(struct wl_listener *listener, void *data);
static void handle_dialog_destroy(struct wl_listener *listener, void *data);
static void connect_extras(xdg_shell *shell);
static void watch_parent_request(void *data, enum wl_protocol_logger_type type,
                                 const struct wl_protocol_logger_message *message);
static xdg_view *view_of_resource(struct wl_resource *resource);
static void connect_optional(struct wl_listener *listener, struct wl_signal *signal,
                             wl_notify_func_t notify);

xdg_shell *xdg_shell_create(compositor *server)
{
    xdg_shell *shell = calloc(1, sizeof(*shell));
    if (shell == NULL) {
        return NULL;
    }
    shell->server = server;
    shell->wlr_shell = wlr_xdg_shell_create(server->display, XDG_SHELL_VERSION);
    if (shell->wlr_shell == NULL) {
        wlr_log(WLR_ERROR, "cannot create the xdg-shell global");
        free(shell);
        return NULL;
    }
    listener_connect(&shell->new_toplevel, &shell->wlr_shell->events.new_toplevel,
                     handle_new_toplevel);
    listener_connect(&shell->new_popup, &shell->wlr_shell->events.new_popup, handle_new_popup);
    connect_extras(shell);
    shell->parent_watch = wl_display_add_protocol_logger(server->display, watch_parent_request,
                                                         shell);
    return shell;
}

void xdg_shell_destroy(xdg_shell *shell)
{
    listener_disconnect(&shell->new_toplevel);
    listener_disconnect(&shell->new_popup);
    listener_disconnect(&shell->new_decoration);
    listener_disconnect(&shell->new_kde_decoration);
    listener_disconnect(&shell->new_dialog);
    if (shell->parent_watch != NULL) {
        wl_protocol_logger_destroy(shell->parent_watch);
    }
    free(shell);
}

static void connect_extras(xdg_shell *shell)
{
    protocol_globals *globals = &shell->server->globals;
    connect_optional(&shell->new_decoration,
                     globals->xdg_decoration != NULL
                         ? &globals->xdg_decoration->events.new_toplevel_decoration
                         : NULL,
                     handle_new_decoration);
    connect_optional(&shell->new_kde_decoration,
                     globals->server_decoration != NULL
                         ? &globals->server_decoration->events.new_decoration
                         : NULL,
                     handle_new_kde_decoration);
    connect_optional(&shell->new_dialog,
                     globals->dialog != NULL ? &globals->dialog->events.new_dialog : NULL,
                     handle_new_dialog);
}

static void watch_parent_request(void *data, enum wl_protocol_logger_type type,
                                 const struct wl_protocol_logger_message *message)
{
    (void) data;
    if (type != WL_PROTOCOL_LOGGER_REQUEST || strcmp(message->message->name, "set_parent") != 0
        || strcmp(wl_resource_get_class(message->resource), toplevel_class) != 0) {
        return;
    }
    xdg_view *child = view_of_resource(message->resource);
    struct wl_resource *parent = (struct wl_resource *) message->arguments[0].o;
    if (child != NULL) {
        xdg_view_parent_requested(child, parent != NULL ? view_of_resource(parent) : NULL);
    }
}

static xdg_view *view_of_resource(struct wl_resource *resource)
{
    return xdg_view_from_toplevel(wlr_xdg_toplevel_from_resource(resource));
}

static void handle_new_toplevel(struct wl_listener *listener, void *data)
{
    xdg_shell *shell = wl_container_of(listener, shell, new_toplevel);
    if (xdg_view_create(shell->server, data) == NULL) {
        wlr_log(WLR_ERROR, "cannot track a new toplevel");
    }
}

static void handle_new_popup(struct wl_listener *listener, void *data)
{
    xdg_shell *shell = wl_container_of(listener, shell, new_popup);
    struct wlr_xdg_popup *xdg_popup = data;
    if (xdg_popup->parent == NULL) {
        return;
    }
    struct wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
    if (parent != NULL && parent->data != NULL) {
        popup_create(shell->server, xdg_popup, parent->data);
    }
}

struct wlr_surface *xdg_shell_popup_grabbed(const xdg_shell *shell,
                                            const struct wlr_seat_keyboard_grab *grab)
{
    struct wlr_xdg_popup_grab *popup_grab;
    wl_list_for_each(popup_grab, &shell->wlr_shell->popup_grabs, link) {
        if (&popup_grab->keyboard_grab == grab && !wl_list_empty(&popup_grab->popups)) {
            struct wlr_xdg_popup *top = wl_container_of(popup_grab->popups.next, top, grab_link);
            return popup_grabbed(top);
        }
    }
    return NULL;
}

static void handle_new_decoration(struct wl_listener *listener, void *data)
{
    (void) listener;
    struct wlr_xdg_toplevel_decoration_v1 *decoration = data;
    xdg_view *xv = xdg_view_from_toplevel(decoration->toplevel);
    if (xv != NULL) {
        xdg_view_attach_decoration(xv, decoration);
    }
}

static void handle_new_kde_decoration(struct wl_listener *listener, void *data)
{
    (void) listener;
    struct wlr_server_decoration *decoration = data;
    struct wlr_xdg_toplevel *toplevel = wlr_xdg_toplevel_try_from_wlr_surface(decoration->surface);
    xdg_view *xv = xdg_view_from_toplevel(toplevel);
    if (xv != NULL) {
        xdg_view_attach_kde_decoration(xv, decoration);
    }
}

static void handle_new_dialog(struct wl_listener *listener, void *data)
{
    (void) listener;
    struct wlr_xdg_dialog_v1 *dialog = data;
    dialog_watch *watch = calloc(1, sizeof(*watch));
    if (watch == NULL) {
        return;
    }
    watch->dialog = dialog;
    listener_connect(&watch->set_modal, &dialog->events.set_modal, handle_dialog_modal);
    listener_connect(&watch->destroy, &dialog->events.destroy, handle_dialog_destroy);
}

static void handle_dialog_modal(struct wl_listener *listener, void *data)
{
    (void) data;
    dialog_watch *watch = wl_container_of(listener, watch, set_modal);
    xdg_view *xv = xdg_view_from_toplevel(watch->dialog->xdg_toplevel);
    if (xv != NULL) {
        xdg_view_modal_changed(xv);
    }
}

static void handle_dialog_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    dialog_watch *watch = wl_container_of(listener, watch, destroy);
    listener_disconnect(&watch->set_modal);
    listener_disconnect(&watch->destroy);
    free(watch);
}

static void connect_optional(struct wl_listener *listener, struct wl_signal *signal,
                             wl_notify_func_t notify)
{
    wl_list_init(&listener->link);
    if (signal != NULL) {
        listener_connect(listener, signal, notify);
    }
}
