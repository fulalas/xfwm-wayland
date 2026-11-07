#ifndef COMPOSITOR_ACTIVATION_H
#define COMPOSITOR_ACTIVATION_H

#include <stdbool.h>

#include "server/server.h"

struct wlr_surface;

activation *activation_create(compositor *server);
void activation_destroy(activation *activation);
bool activation_allows_map(const activation *activation, const struct wlr_surface *surface);
const char *activation_launch_of(const activation *activation,
                                 const struct wlr_surface *surface);

#endif
