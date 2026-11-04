#include <wlr/backend.h>
#include <wlr/render/drm_syncobj.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_linux_drm_syncobj_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>

#include "render_sync.h"

enum {
    SYNCOBJ_VERSION = 1,
};

bool render_sync_supported(const compositor *server)
{
    return server->renderer->features.timeline && server->backend->features.timeline &&
           wlr_renderer_get_drm_fd(server->renderer) >= 0;
}

void render_sync_create_global(compositor *server)
{
    if (!render_sync_supported(server)) {
        wlr_log(WLR_INFO, "explicit sync is not offered: no timeline support");
        return;
    }
    int drm_fd = wlr_renderer_get_drm_fd(server->renderer);
    if (wlr_linux_drm_syncobj_manager_v1_create(server->display, SYNCOBJ_VERSION, drm_fd) ==
        NULL) {
        wlr_log(WLR_ERROR, "cannot create the explicit sync global");
    }
}

struct wlr_drm_syncobj_timeline *render_sync_timeline_create(const compositor *server)
{
    if (!render_sync_supported(server)) {
        return NULL;
    }
    struct wlr_drm_syncobj_timeline *timeline =
        wlr_drm_syncobj_timeline_create(wlr_renderer_get_drm_fd(server->renderer));
    if (timeline == NULL) {
        wlr_log(WLR_ERROR, "cannot create a render timeline");
    }
    return timeline;
}

void render_sync_drop_wait(struct wlr_output_state *state)
{
    wlr_drm_syncobj_timeline_unref(state->wait_timeline);
    state->wait_timeline = NULL;
    state->wait_point = 0;
    state->committed &= ~(uint32_t) WLR_OUTPUT_STATE_WAIT_TIMELINE;
}
