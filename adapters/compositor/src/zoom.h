#ifndef COMPOSITOR_ZOOM_H
#define COMPOSITOR_ZOOM_H

#include <stdbool.h>

#include "output.h"
#include "server.h"
#include "wm/views.h"

zoom *zoom_create(compositor *server);
void zoom_destroy(zoom *zoom);
void zoom_set(zoom *zoom, const wm_zoom_view *view);
bool zoom_is_active(const zoom *zoom);
void zoom_render(zoom *zoom, output *output);
void zoom_release(output *output);

#endif
