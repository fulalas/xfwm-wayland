#ifndef COMPOSITOR_OUTPUT_HEAD_H
#define COMPOSITOR_OUTPUT_HEAD_H

#include <wayland-server-core.h>

#include "edid.h"
#include "output.h"

typedef struct output_head {
    struct wlr_output *wlr_output;
    char *name;
    edid_blob edid;
    struct wl_list resources;
    struct wl_list link;
    struct wl_listener output_destroy;
} output_head;

output_head *output_head_create(output *output, struct wl_list *heads);
void output_head_announce(output_head *head, struct wl_resource *manager);
void output_head_destroy(output_head *head);

#endif
