#ifndef COMPOSITOR_DISPLAYS_MODULE_H
#define COMPOSITOR_DISPLAYS_MODULE_H

#include <stdbool.h>

#include "output.h"
#include "server.h"

struct wl_client;

displays *displays_create(compositor *server);
void displays_destroy(displays *displays);
void displays_output_added(displays *displays, output *output);
void displays_output_removed(displays *displays);
void displays_start(displays *displays);
void displays_set_lid_closed(displays *displays, bool closed);
void displays_configured(displays *displays, struct wl_client *client);
void displays_reload_primary(displays *displays);

#endif
