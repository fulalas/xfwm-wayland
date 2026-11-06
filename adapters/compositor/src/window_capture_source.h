#ifndef COMPOSITOR_WINDOW_CAPTURE_SOURCE_H
#define COMPOSITOR_WINDOW_CAPTURE_SOURCE_H

#include "server.h"
#include "view.h"

struct wlr_ext_image_capture_source_v1;

typedef struct window_capture_source window_capture_source;

window_capture_source *window_capture_source_create(compositor *server, view *view);
void window_capture_source_destroy(window_capture_source *source);
struct wlr_ext_image_capture_source_v1 *window_capture_source_base(window_capture_source *source);
view *window_capture_source_view(const window_capture_source *source);

#endif
