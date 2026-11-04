#ifndef COMPOSITOR_PRESENT_SYNC_H
#define COMPOSITOR_PRESENT_SYNC_H

#include "server.h"

typedef struct present_sync present_sync;

present_sync *present_sync_create(compositor *server);
void present_sync_destroy(present_sync *sync);

#endif
