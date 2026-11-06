#ifndef COMPOSITOR_XSETTINGS_TABLE_H
#define COMPOSITOR_XSETTINGS_TABLE_H

#include <stddef.h>

#include "compositor/x11_settings.h"

typedef struct xsettings_table xsettings_table;

xsettings_table *xsettings_table_create(void);
void xsettings_table_destroy(xsettings_table *table);
void xsettings_table_reset(xsettings_table *table);
void xsettings_table_set_scale(xsettings_table *table, double scale);
void xsettings_table_add(xsettings_table *table, const x11_setting *setting);
size_t xsettings_table_count(const xsettings_table *table);
const x11_setting *xsettings_table_at(const xsettings_table *table, size_t index);
const x11_setting *xsettings_table_find(const xsettings_table *table, const char *name);

#endif
