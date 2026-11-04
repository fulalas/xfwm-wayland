#include <glib.h>
#include <stdlib.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "listener.h"
#include "session_file.h"
#include "view.h"
#include "xdg-session-management-v1-protocol.h"
#include "xdg_session_private.h"

enum {
    MANAGER_VERSION = 1,
    SAVE_DELAY_MS = 1000,
};

static void manager_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void manager_handle_destroy(struct wl_client *client, struct wl_resource *resource);
static void manager_get_session(struct wl_client *client, struct wl_resource *resource,
                                uint32_t id, uint32_t reason, const char *session_id);
static bool session_request_valid(struct wl_resource *manager, uint32_t reason,
                                  const char *session_id);
static session_data *session_find(const xdg_sessions *sessions, const char *id);
static session_data *session_open(xdg_sessions *sessions, struct wl_resource *resource,
                                  const char *wanted_id);
static bool session_load(session_data *session, const char *wanted_id);
static void session_take_over(session_data *session);
static void session_close(session_data *session);
static int handle_save_timer(void *data);
static void handle_view_changed(struct wl_listener *listener, void *data);
static void session_handle_resource_destroy(struct wl_resource *resource);
static void session_handle_destroy(struct wl_client *client, struct wl_resource *resource);
static void session_handle_remove(struct wl_client *client, struct wl_resource *resource);
static void session_handle_add_toplevel(struct wl_client *client, struct wl_resource *resource,
                                        uint32_t id, struct wl_resource *toplevel,
                                        const char *name);
static void session_handle_restore_toplevel(struct wl_client *client,
                                            struct wl_resource *resource, uint32_t id,
                                            struct wl_resource *toplevel, const char *name);
static void session_handle_remove_toplevel(struct wl_client *client,
                                           struct wl_resource *resource, const char *name);

static const struct xdg_session_manager_v1_interface manager_impl = {
    .destroy = manager_handle_destroy,
    .get_session = manager_get_session,
};

static const struct xdg_session_v1_interface session_impl = {
    .destroy = session_handle_destroy,
    .remove = session_handle_remove,
    .add_toplevel = session_handle_add_toplevel,
    .restore_toplevel = session_handle_restore_toplevel,
    .remove_toplevel = session_handle_remove_toplevel,
};

xdg_sessions *xdg_sessions_create(compositor *server)
{
    xdg_sessions *sessions = calloc(1, sizeof(*sessions));
    if (sessions == NULL) {
        return NULL;
    }
    sessions->server = server;
    wl_list_init(&sessions->sessions);
    sessions->global = wl_global_create(server->display, &xdg_session_manager_v1_interface,
                                        MANAGER_VERSION, sessions, manager_bind);
    if (sessions->global == NULL) {
        wlr_log(WLR_ERROR, "cannot create the xdg session management global");
        free(sessions);
        return NULL;
    }
    listener_connect(&sessions->view_changed, &server->view_events.changed, handle_view_changed);
    session_file_evict_expired();
    return sessions;
}

void xdg_sessions_destroy(xdg_sessions *sessions)
{
    session_data *session;
    session_data *tmp;
    wl_list_for_each_safe(session, tmp, &sessions->sessions, link) {
        wl_resource_set_user_data(session->resource, NULL);
        session_close(session);
    }
    listener_disconnect(&sessions->view_changed);
    wl_global_destroy(sessions->global);
    free(sessions);
}

void session_data_changed(session_data *session)
{
    if (session->save_timer != NULL) {
        return;
    }
    struct wl_event_loop *loop = session->owner->server->loop;
    session->save_timer = wl_event_loop_add_timer(loop, handle_save_timer, session);
    if (session->save_timer != NULL) {
        wl_event_source_timer_update(session->save_timer, SAVE_DELAY_MS);
    }
}

static void manager_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    struct wl_resource *resource =
        wl_resource_create(client, &xdg_session_manager_v1_interface, (int) version, id);
    if (resource == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &manager_impl, data, NULL);
}

static void manager_handle_destroy(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    wl_resource_destroy(resource);
}

static void manager_get_session(struct wl_client *client, struct wl_resource *resource,
                                uint32_t id, uint32_t reason, const char *session_id)
{
    xdg_sessions *sessions = wl_resource_get_user_data(resource);
    if (!session_request_valid(resource, reason, session_id)) {
        return;
    }
    session_data *existing = session_id != NULL ? session_find(sessions, session_id) : NULL;
    if (existing != NULL && wl_resource_get_client(existing->resource) == client) {
        wl_resource_post_error(resource, XDG_SESSION_MANAGER_V1_ERROR_IN_USE,
                               "session %s is already in use", session_id);
        return;
    }
    struct wl_resource *session_resource = wl_resource_create(
        client, &xdg_session_v1_interface, wl_resource_get_version(resource), id);
    if (session_resource == NULL) {
        wl_client_post_no_memory(client);
        return;
    }
    if (existing != NULL) {
        session_take_over(existing);
    }
    session_data *session = session_open(sessions, session_resource, session_id);
    if (session != NULL) {
        session->reason = reason;
    }
}

static bool session_request_valid(struct wl_resource *manager, uint32_t reason,
                                  const char *session_id)
{
    if (!xdg_session_manager_v1_reason_is_valid(reason, wl_resource_get_version(manager))) {
        wl_resource_post_error(manager, XDG_SESSION_MANAGER_V1_ERROR_INVALID_REASON,
                               "invalid reason %u", reason);
        return false;
    }
    if (session_id != NULL && !g_utf8_validate(session_id, -1, NULL)) {
        wl_resource_post_error(manager, XDG_SESSION_MANAGER_V1_ERROR_INVALID_SESSION_ID,
                               "session id is not UTF-8");
        return false;
    }
    return true;
}

static session_data *session_find(const xdg_sessions *sessions, const char *id)
{
    session_data *session;
    wl_list_for_each(session, &sessions->sessions, link) {
        if (id != NULL && g_strcmp0(session->id, id) == 0) {
            return session;
        }
    }
    return NULL;
}

static session_data *session_open(xdg_sessions *sessions, struct wl_resource *resource,
                                  const char *wanted_id)
{
    session_data *session = calloc(1, sizeof(*session));
    if (session == NULL) {
        wl_resource_post_no_memory(resource);
        return NULL;
    }
    session->owner = sessions;
    session->resource = resource;
    wl_list_init(&session->toplevels);
    wl_resource_set_implementation(resource, &session_impl, session,
                                   session_handle_resource_destroy);
    wl_list_insert(&sessions->sessions, &session->link);
    if (session_load(session, wanted_id)) {
        xdg_session_v1_send_restored(resource);
    } else {
        xdg_session_v1_send_created(resource, session->id);
    }
    session_data_changed(session);
    return session;
}

static bool session_load(session_data *session, const char *wanted_id)
{
    if (wanted_id != NULL && session_file_load(wanted_id, &session->book)) {
        session->id = g_strdup(wanted_id);
        return true;
    }
    session_book_finish(&session->book);
    session->id = session_file_new_id();
    return false;
}

static void session_take_over(session_data *session)
{
    xdg_session_v1_send_replaced(session->resource);
    wl_resource_set_user_data(session->resource, NULL);
    session_close(session);
}

static void session_close(session_data *session)
{
    session_toplevel *toplevel;
    session_toplevel *tmp;
    wl_list_for_each_safe(toplevel, tmp, &session->toplevels, link) {
        session_toplevel_make_inert(toplevel);
    }
    if (session->save_timer != NULL) {
        wl_event_source_remove(session->save_timer);
        session_file_save(session->id, &session->book);
    }
    wl_list_remove(&session->link);
    session_book_finish(&session->book);
    g_free(session->id);
    free(session);
}

static int handle_save_timer(void *data)
{
    session_data *session = data;
    wl_event_source_remove(session->save_timer);
    session->save_timer = NULL;
    session_book_trim(&session->book, SESSION_BOOK_MAX);
    session_file_save(session->id, &session->book);
    return 0;
}

static void handle_view_changed(struct wl_listener *listener, void *data)
{
    xdg_sessions *sessions = wl_container_of(listener, sessions, view_changed);
    view *v = data;
    struct wlr_surface *surface = view_surface(v);
    struct wlr_xdg_toplevel *toplevel =
        surface != NULL ? wlr_xdg_toplevel_try_from_wlr_surface(surface) : NULL;
    session_toplevel *tracked = toplevel != NULL ? session_toplevel_of(sessions, toplevel) : NULL;
    if (tracked != NULL) {
        session_toplevel_store(tracked, view_state(v));
    }
}

static void session_handle_resource_destroy(struct wl_resource *resource)
{
    session_data *session = wl_resource_get_user_data(resource);
    if (session != NULL) {
        session_close(session);
    }
}

static void session_handle_destroy(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    wl_resource_destroy(resource);
}

static void session_handle_remove(struct wl_client *client, struct wl_resource *resource)
{
    (void) client;
    session_data *session = wl_resource_get_user_data(resource);
    if (session != NULL) {
        session_file_remove(session->id);
        session_book_finish(&session->book);
        if (session->save_timer != NULL) {
            wl_event_source_remove(session->save_timer);
            session->save_timer = NULL;
        }
    }
    wl_resource_destroy(resource);
}

static void session_handle_add_toplevel(struct wl_client *client, struct wl_resource *resource,
                                        uint32_t id, struct wl_resource *toplevel,
                                        const char *name)
{
    (void) client;
    toplevel_request request = {resource, id, toplevel, name};
    session_toplevel_add(&request);
}

static void session_handle_restore_toplevel(struct wl_client *client,
                                            struct wl_resource *resource, uint32_t id,
                                            struct wl_resource *toplevel, const char *name)
{
    (void) client;
    toplevel_request request = {resource, id, toplevel, name};
    session_toplevel_restore(&request);
}

static void session_handle_remove_toplevel(struct wl_client *client,
                                           struct wl_resource *resource, const char *name)
{
    (void) client;
    session_data *session = wl_resource_get_user_data(resource);
    if (session != NULL && session_book_remove(&session->book, name)) {
        session_data_changed(session);
    }
    if (session != NULL) {
        session_toplevel_forget(session, name);
    }
}
