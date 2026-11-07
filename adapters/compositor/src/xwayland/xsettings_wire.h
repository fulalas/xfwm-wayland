#ifndef COMPOSITOR_XSETTINGS_WIRE_H
#define COMPOSITOR_XSETTINGS_WIRE_H

#include <stddef.h>
#include <stdint.h>

#include "xsettings_table.h"

typedef struct xsettings_bytes {
    uint8_t *data;
    size_t length;
} xsettings_bytes;

xsettings_bytes xsettings_wire_encode(const xsettings_table *table, uint32_t serial);
void xsettings_bytes_free(xsettings_bytes *bytes);

#endif
