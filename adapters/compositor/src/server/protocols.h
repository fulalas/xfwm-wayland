#ifndef COMPOSITOR_PROTOCOLS_H
#define COMPOSITOR_PROTOCOLS_H

#include <stdbool.h>

#include "server.h"

bool protocols_create(compositor *server);
bool protocols_create_input(compositor *server);
bool protocols_create_desktop(compositor *server);
void protocols_restrict_sandboxed(compositor *server);
void protocols_create_color(compositor *server);

#endif
