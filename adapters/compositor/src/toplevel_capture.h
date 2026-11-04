#ifndef COMPOSITOR_TOPLEVEL_CAPTURE_H
#define COMPOSITOR_TOPLEVEL_CAPTURE_H

#include "server.h"

toplevel_capture *toplevel_capture_create(compositor *server);
void toplevel_capture_destroy(toplevel_capture *capture);
void toplevel_capture_drop_sources(toplevel_capture *capture);

#endif
