#ifndef COMPOSITOR_DISPLAY_MODULES_H
#define COMPOSITOR_DISPLAY_MODULES_H

#include <stdbool.h>

#include "server.h"

bool display_modules_create(compositor *server);
void display_modules_destroy(compositor *server);

#endif
