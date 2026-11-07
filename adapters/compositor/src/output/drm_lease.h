#ifndef COMPOSITOR_DRM_LEASE_H
#define COMPOSITOR_DRM_LEASE_H

#include "server/server.h"

typedef struct drm_lease drm_lease;

drm_lease *drm_lease_create(compositor *server);
void drm_lease_destroy(drm_lease *lease);

#endif
