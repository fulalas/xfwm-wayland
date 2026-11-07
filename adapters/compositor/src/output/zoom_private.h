#ifndef COMPOSITOR_ZOOM_PRIVATE_H
#define COMPOSITOR_ZOOM_PRIVATE_H

#include <stdbool.h>

#include "server/server.h"
#include "wm/views.h"

struct zoom {
    compositor *server;
    wm_zoom_view view;
    bool active;
};

#endif
