#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_drm_lease_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include "drm_lease.h"
#include "listener.h"
#include "lock.h"

struct drm_lease {
    compositor *server;
    struct wlr_drm_lease_v1_manager *manager;
    struct wl_listener new_output;
    struct wl_listener request;
    struct wl_listener manager_destroy;
};

static void handle_new_output(struct wl_listener *listener, void *data);
static void handle_request(struct wl_listener *listener, void *data);
static void handle_manager_destroy(struct wl_listener *listener, void *data);
static bool may_grant(const drm_lease *lease);

drm_lease *drm_lease_create(compositor *server)
{
    drm_lease *lease = calloc(1, sizeof(*lease));
    if (lease == NULL) {
        return NULL;
    }
    lease->server = server;
    wl_list_init(&lease->new_output.link);
    wl_list_init(&lease->request.link);
    wl_list_init(&lease->manager_destroy.link);
    lease->manager = wlr_drm_lease_v1_manager_create(server->display, server->backend);
    if (lease->manager == NULL) {
        wlr_log(WLR_INFO, "DRM leasing is not offered: no DRM backend");
        return lease;
    }
    listener_connect(&lease->new_output, &server->backend->events.new_output, handle_new_output);
    listener_connect(&lease->request, &lease->manager->events.request, handle_request);
    listener_connect(&lease->manager_destroy, &lease->manager->events.destroy,
                     handle_manager_destroy);
    return lease;
}

void drm_lease_destroy(drm_lease *lease)
{
    listener_disconnect(&lease->new_output);
    listener_disconnect(&lease->request);
    listener_disconnect(&lease->manager_destroy);
    free(lease);
}

static void handle_new_output(struct wl_listener *listener, void *data)
{
    drm_lease *lease = wl_container_of(listener, lease, new_output);
    struct wlr_output *wlr_output = data;
    if (!wlr_output->non_desktop) {
        return;
    }
    if (!wlr_drm_lease_v1_manager_offer_output(lease->manager, wlr_output)) {
        wlr_log(WLR_ERROR, "cannot offer output %s for leasing", wlr_output->name);
    }
}

static void handle_request(struct wl_listener *listener, void *data)
{
    drm_lease *lease = wl_container_of(listener, lease, request);
    struct wlr_drm_lease_request_v1 *request = data;
    if (request->invalid) {
        return;
    }
    if (!may_grant(lease)) {
        wlr_log(WLR_INFO, "DRM lease request rejected: the session is locked");
        wlr_drm_lease_request_v1_reject(request);
        return;
    }
    if (wlr_drm_lease_request_v1_grant(request) == NULL) {
        wlr_log(WLR_ERROR, "cannot grant a DRM lease");
    }
}

static void handle_manager_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    drm_lease *lease = wl_container_of(listener, lease, manager_destroy);
    listener_disconnect(&lease->new_output);
    listener_disconnect(&lease->request);
    listener_disconnect(&lease->manager_destroy);
    lease->manager = NULL;
}

static bool may_grant(const drm_lease *lease)
{
    session_lock *lock = lease->server->lock;
    return lock == NULL || !session_lock_is_locked(lock);
}
