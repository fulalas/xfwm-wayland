#ifndef COMPOSITOR_PRESENT_SYNC_H
#define COMPOSITOR_PRESENT_SYNC_H

#include "present_rules.h"
#include "server/server.h"

struct wlr_output;

typedef struct present_sync present_sync;

present_sync *present_sync_create(compositor *server);
void present_sync_destroy(present_sync *sync);
const vblank_clock *present_sync_clock(const present_sync *sync, const struct wlr_output *output);

#endif
