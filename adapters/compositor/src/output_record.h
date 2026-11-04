#ifndef COMPOSITOR_OUTPUT_RECORD_H
#define COMPOSITOR_OUTPUT_RECORD_H

#include <stdbool.h>
#include <wayland-server-core.h>

#include "output.h"

struct wlr_seat;

typedef struct output_record {
    struct wl_resource *resource;
    output *output;
    wm_rect workarea;
    bool sent_primary;
    bool pointer_inside;
    struct wl_list link;
    struct wl_listener output_destroy;
} output_record;

output_record *output_record_create(struct wl_resource *resource, struct wl_list *records);
void output_record_attach(output_record *record, output *output);
void output_record_send_changes(output_record *record);
void output_record_leave_unless(output_record *record, output *target, struct wlr_seat *seat);
void output_record_enter_if(output_record *record, output *target, struct wlr_seat *seat);
void output_record_orphan(output_record *record);

#endif
