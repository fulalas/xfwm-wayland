#ifndef COMPOSITOR_RENDER_SYNC_H
#define COMPOSITOR_RENDER_SYNC_H

#include <stdbool.h>

#include "server.h"

struct wlr_drm_syncobj_timeline;
struct wlr_output_state;

void render_sync_create_global(compositor *server);
struct wlr_drm_syncobj_timeline *render_sync_timeline_create(const compositor *server);
void render_sync_drop_wait(struct wlr_output_state *state);

#endif
