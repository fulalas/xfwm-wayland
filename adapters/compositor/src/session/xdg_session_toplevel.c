#include <glib.h>
#include <stdlib.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "util/listener.h"
#include "util/resource.h"
#include "session_apply.h"
#include "shell/view.h"
#include "xdg-session-management-v1-protocol.h"
#include "xdg_session_private.h"

static bool request_is_valid(session_data *session, const toplevel_request *request,
                             const struct wlr_xdg_toplevel *toplevel);
static bool name_is_live(const session_data *session, const char *name);
static bool toplevel_is_added(const session_data *session,
                              const struct wlr_xdg_toplevel *toplevel);
static void track_toplevel(session_data *session, const toplevel_request *request,
                           struct wlr_xdg_toplevel *toplevel);
static session_toplevel *toplevel_create(const toplevel_request *request);
static void toplevel_attach(session_toplevel *tracked, session_data *session,
                            struct wlr_xdg_toplevel *toplevel);
static session_toplevel *session_toplevel_in(session_data *session,
                                             const struct wlr_xdg_toplevel *toplevel);
static void store_current_state(session_toplevel *tracked);
static void begin_restore(session_toplevel *tracked, const session_record *saved);
static void handle_toplevel_destroy(struct wl_listener *listener, void *data);
static void handle_surface_commit(struct wl_listener *listener, void *data);
static void handle_resource_gone(struct wl_resource *resource);
static void handle_rename(struct wl_client *client, struct wl_resource *resource,
                          const char *name);

static const struct xdg_toplevel_session_v1_interface toplevel_session_impl = {
    .destroy = resource_handle_destroy,
    .rename = handle_rename,
};

void session_toplevel_add(const toplevel_request *request)
{
    session_data *session = wl_resource_get_user_data(request->session);
    struct wlr_xdg_toplevel *toplevel = wlr_xdg_toplevel_from_resource(request->toplevel);
    if (session == NULL || toplevel == NULL) {
        toplevel_create(request);
        return;
    }
    if (session_book_find(&session->book, request->name) != NULL) {
        wl_resource_post_error(request->session, XDG_SESSION_V1_ERROR_NAME_IN_USE,
                               "toplevel name is already known");
        return;
    }
    if (!request_is_valid(session, request, toplevel)) {
        return;
    }
    session_toplevel *tracked = toplevel_create(request);
    if (tracked != NULL) {
        toplevel_attach(tracked, session, toplevel);
        store_current_state(tracked);
    }
}

void session_toplevel_restore(const toplevel_request *request)
{
    session_data *session = wl_resource_get_user_data(request->session);
    struct wlr_xdg_toplevel *toplevel = wlr_xdg_toplevel_from_resource(request->toplevel);
    if (session == NULL || toplevel == NULL) {
        toplevel_create(request);
        return;
    }
    if (toplevel->base->initialized) {
        wl_resource_post_error(request->session, XDG_SESSION_V1_ERROR_ALREADY_MAPPED,
                               "toplevel was already committed");
        return;
    }
    if (request_is_valid(session, request, toplevel)) {
        track_toplevel(session, request, toplevel);
    }
}

static void track_toplevel(session_data *session, const toplevel_request *request,
                           struct wlr_xdg_toplevel *toplevel)
{
    session_toplevel *tracked = toplevel_create(request);
    if (tracked == NULL) {
        return;
    }
    toplevel_attach(tracked, session, toplevel);
    const session_record *saved = session_book_find(&session->book, request->name);
    if (saved != NULL) {
        begin_restore(tracked, saved);
    }
}

void session_toplevel_forget(session_data *session, const char *name)
{
    session_toplevel *tracked;
    session_toplevel *tmp;
    wl_list_for_each_safe(tracked, tmp, &session->toplevels, link) {
        if (g_strcmp0(tracked->name, name) == 0) {
            session_toplevel_make_inert(tracked);
        }
    }
}

void session_toplevel_make_inert(session_toplevel *tracked)
{
    if (tracked->resource != NULL) {
        wl_resource_set_user_data(tracked->resource, NULL);
    }
    listener_disconnect(&tracked->toplevel_destroy);
    listener_disconnect(&tracked->surface_commit);
    wl_list_remove(&tracked->link);
    g_free(tracked->name);
    free(tracked);
}

session_toplevel *session_toplevel_of(const xdg_sessions *sessions,
                                      const struct wlr_xdg_toplevel *toplevel)
{
    session_data *session;
    wl_list_for_each(session, &sessions->sessions, link) {
        session_toplevel *tracked = session_toplevel_in(session, toplevel);
        if (tracked != NULL) {
            return tracked;
        }
    }
    return NULL;
}

static session_toplevel *session_toplevel_in(session_data *session,
                                             const struct wlr_xdg_toplevel *toplevel)
{
    session_toplevel *tracked;
    wl_list_for_each(tracked, &session->toplevels, link) {
        if (tracked->toplevel == toplevel) {
            return tracked;
        }
    }
    return NULL;
}

void session_toplevel_store(session_toplevel *tracked, const wm_window_view *view)
{
    session_record record = session_record_from_view(view);
    record.name = tracked->name;
    record.seen = g_get_real_time() / G_USEC_PER_SEC;
    session_data *session = tracked->session;
    const session_record *known = session_book_find(&session->book, tracked->name);
    if (known != NULL && session_record_same_state(known, &record)) {
        return;
    }
    if (session_book_put(&session->book, &record)) {
        session_data_changed(session);
    }
}

static bool request_is_valid(session_data *session, const toplevel_request *request,
                             const struct wlr_xdg_toplevel *toplevel)
{
    if (!g_utf8_validate(request->name, -1, NULL)) {
        wl_resource_post_error(request->session, XDG_SESSION_V1_ERROR_INVALID_NAME,
                               "toplevel name is not UTF-8");
        return false;
    }
    if (name_is_live(session, request->name)) {
        wl_resource_post_error(request->session, XDG_SESSION_V1_ERROR_NAME_IN_USE,
                               "toplevel name is used by another toplevel");
        return false;
    }
    if (toplevel_is_added(session, toplevel)) {
        wl_resource_post_error(request->session, XDG_SESSION_V1_ERROR_ALREADY_ADDED,
                               "toplevel was already added to a session");
        return false;
    }
    return true;
}

static bool name_is_live(const session_data *session, const char *name)
{
    session_toplevel *tracked;
    wl_list_for_each(tracked, &session->toplevels, link) {
        if (g_strcmp0(tracked->name, name) == 0) {
            return true;
        }
    }
    return false;
}

static bool toplevel_is_added(const session_data *session,
                              const struct wlr_xdg_toplevel *toplevel)
{
    session_toplevel *tracked = session_toplevel_of(session->owner, toplevel);
    return tracked != NULL && wl_resource_get_client(tracked->session->resource) ==
                                  wl_resource_get_client(session->resource);
}

static session_toplevel *toplevel_create(const toplevel_request *request)
{
    struct wl_resource *resource =
        wl_resource_create(wl_resource_get_client(request->session),
                           &xdg_toplevel_session_v1_interface,
                           wl_resource_get_version(request->session), request->id);
    session_toplevel *tracked = resource != NULL ? calloc(1, sizeof(*tracked)) : NULL;
    if (tracked == NULL) {
        wl_client_post_no_memory(wl_resource_get_client(request->session));
        return NULL;
    }
    tracked->resource = resource;
    tracked->name = g_strdup(request->name);
    wl_list_init(&tracked->link);
    wl_list_init(&tracked->toplevel_destroy.link);
    wl_list_init(&tracked->surface_commit.link);
    wl_resource_set_implementation(resource, &toplevel_session_impl, tracked,
                                   handle_resource_gone);
    return tracked;
}

static void toplevel_attach(session_toplevel *tracked, session_data *session,
                            struct wlr_xdg_toplevel *toplevel)
{
    tracked->session = session;
    tracked->toplevel = toplevel;
    wl_list_insert(&session->toplevels, &tracked->link);
    listener_connect(&tracked->toplevel_destroy, &toplevel->events.destroy,
                     handle_toplevel_destroy);
}

static void store_current_state(session_toplevel *tracked)
{
    compositor *server = tracked->session->owner->server;
    view *v = views_find_by_surface(server, tracked->toplevel->base->surface);
    if (v != NULL && view_window(v) != NULL) {
        session_toplevel_store(tracked, view_state(v));
    }
}

static void begin_restore(session_toplevel *tracked, const session_record *saved)
{
    tracked->restore = *saved;
    tracked->restore.name = NULL;
    tracked->restore_pending = true;
    listener_connect(&tracked->surface_commit, &tracked->toplevel->base->surface->events.commit,
                     handle_surface_commit);
}

static void handle_toplevel_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    session_toplevel *tracked = wl_container_of(listener, tracked, toplevel_destroy);
    session_toplevel_make_inert(tracked);
}

static void handle_surface_commit(struct wl_listener *listener, void *data)
{
    (void) data;
    session_toplevel *tracked = wl_container_of(listener, tracked, surface_commit);
    if (!tracked->toplevel->base->initial_commit) {
        return;
    }
    listener_disconnect(&tracked->surface_commit);
    if (tracked->resource != NULL) {
        xdg_toplevel_session_v1_send_restored(tracked->resource);
    }
}

static void handle_resource_gone(struct wl_resource *resource)
{
    session_toplevel *tracked = wl_resource_get_user_data(resource);
    if (tracked == NULL) {
        return;
    }
    tracked->resource = NULL;
    if (tracked->session == NULL) {
        session_toplevel_make_inert(tracked);
    }
}

static void handle_rename(struct wl_client *client, struct wl_resource *resource,
                          const char *name)
{
    (void) client;
    session_toplevel *tracked = wl_resource_get_user_data(resource);
    if (tracked == NULL || tracked->session == NULL || g_strcmp0(tracked->name, name) == 0) {
        return;
    }
    session_data *session = tracked->session;
    if (name_is_live(session, name) || session_book_find(&session->book, name) != NULL) {
        wl_resource_post_error(session->resource, XDG_SESSION_V1_ERROR_NAME_IN_USE,
                               "toplevel name is already in use");
        return;
    }
    if (session_book_rename(&session->book, tracked->name, name)) {
        session_data_changed(session);
    }
    g_free(tracked->name);
    tracked->name = g_strdup(name);
}
