#ifndef COMPOSITOR_SERVER_MODULES_H
#define COMPOSITOR_SERVER_MODULES_H

#include <stdbool.h>

#include "server.h"

bool server_modules_create(compositor *server);
void server_modules_destroy(compositor *server);

#endif
