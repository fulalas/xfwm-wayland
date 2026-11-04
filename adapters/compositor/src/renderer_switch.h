#ifndef COMPOSITOR_RENDERER_SWITCH_H
#define COMPOSITOR_RENDERER_SWITCH_H

#include <stdbool.h>

#include "server.h"

bool renderer_create_initial(compositor *server);
void renderer_follow_config(compositor *server);
bool renderer_fall_back(compositor *server);
void renderer_release(compositor *server);

#endif
