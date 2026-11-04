#ifndef COMPOSITOR_OSD_PREVIEW_H
#define COMPOSITOR_OSD_PREVIEW_H

#include <stdbool.h>

#include "server.h"

struct wlr_scene_buffer;
struct wlr_scene_tree;
struct wlr_surface;

typedef struct osd_preview osd_preview;

osd_preview *osd_preview_create(compositor *server, struct wlr_scene_tree *parent);
void osd_preview_destroy(osd_preview *preview);
bool osd_preview_show(osd_preview *preview, struct wlr_surface *surface);
void osd_preview_clear(osd_preview *preview);
struct wlr_scene_buffer *osd_preview_node(const osd_preview *preview);

#endif
