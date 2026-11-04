#ifndef COMPOSITOR_ACCESSIBILITY_H
#define COMPOSITOR_ACCESSIBILITY_H

#include "server.h"

accessibility *accessibility_create(compositor *server);
void accessibility_destroy(accessibility *accessibility);

#endif
