#ifndef COMPOSITOR_EDID_H
#define COMPOSITOR_EDID_H

#include <stddef.h>
#include <stdint.h>

struct wlr_output;

typedef struct edid_blob {
    uint8_t *data;
    size_t size;
} edid_blob;

edid_blob edid_read(struct wlr_output *output);
void edid_release(edid_blob *blob);
int edid_create_fd(const edid_blob *blob);

#endif
