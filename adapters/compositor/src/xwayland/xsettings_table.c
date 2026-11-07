#include "xsettings_table.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum {
    TABLE_CAPACITY = 128,
    XWAYLAND_WINDOW_SCALE = 1,
    XWAYLAND_DPI = X11_SETTING_UNSCALED_DPI * X11_SETTING_XFT_DPI_UNIT,
};

static const char window_scaling_factor[] = "Gdk/WindowScalingFactor";
static const char *const ignored_names[] = {window_scaling_factor, "Gdk/UnscaledDPI"};

struct xsettings_table {
    x11_setting items[TABLE_CAPACITY];
    size_t count;
    double scale;
};

static bool is_ignored(const char *name);
static void release(x11_setting *setting);
static x11_setting copy_of(const x11_setting *setting);
static void store(xsettings_table *table, const x11_setting *setting);

xsettings_table *xsettings_table_create(void)
{
    xsettings_table *table = calloc(1, sizeof(*table));
    if (table != NULL) {
        table->scale = 1.0;
        xsettings_table_reset(table);
    }
    return table;
}

void xsettings_table_destroy(xsettings_table *table)
{
    for (size_t i = 0; i < table->count; i++) {
        release(&table->items[i]);
    }
    free(table);
}

void xsettings_table_reset(xsettings_table *table)
{
    for (size_t i = 0; i < table->count; i++) {
        release(&table->items[i]);
    }
    table->count = 0;
    x11_setting scale = {window_scaling_factor, X11_SETTING_INTEGER, XWAYLAND_WINDOW_SCALE, NULL};
    x11_setting dpi = {X11_SETTING_XFT_DPI, X11_SETTING_INTEGER, XWAYLAND_DPI, NULL};
    store(table, &scale);
    store(table, &dpi);
}

void xsettings_table_set_scale(xsettings_table *table, double scale)
{
    table->scale = scale > 0.0 ? scale : 1.0;
}

void xsettings_table_add(xsettings_table *table, const x11_setting *setting)
{
    if (setting->name == NULL || is_ignored(setting->name)) {
        return;
    }
    if (setting->type == X11_SETTING_STRING && setting->string == NULL) {
        return;
    }
    if (setting->type == X11_SETTING_INTEGER && strcmp(setting->name, X11_SETTING_XFT_DPI) == 0) {
        x11_setting dpi = *setting;
        dpi.integer = (int32_t) lround(setting->integer / table->scale);
        store(table, &dpi);
        return;
    }
    store(table, setting);
}

size_t xsettings_table_count(const xsettings_table *table)
{
    return table->count;
}

const x11_setting *xsettings_table_at(const xsettings_table *table, size_t index)
{
    return &table->items[index];
}

const x11_setting *xsettings_table_find(const xsettings_table *table, const char *name)
{
    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->items[i].name, name) == 0) {
            return &table->items[i];
        }
    }
    return NULL;
}

static bool is_ignored(const char *name)
{
    for (size_t i = 0; i < sizeof(ignored_names) / sizeof(ignored_names[0]); i++) {
        if (strcmp(name, ignored_names[i]) == 0) {
            return true;
        }
    }
    return false;
}

static void release(x11_setting *setting)
{
    free((char *) setting->name);
    free((char *) setting->string);
}

static x11_setting copy_of(const x11_setting *setting)
{
    x11_setting copy = *setting;
    copy.name = strdup(setting->name);
    copy.string = setting->type == X11_SETTING_STRING ? strdup(setting->string) : NULL;
    return copy;
}

static void store(xsettings_table *table, const x11_setting *setting)
{
    x11_setting copy = copy_of(setting);
    bool complete = copy.name != NULL && (copy.type != X11_SETTING_STRING || copy.string != NULL);
    x11_setting *existing = (x11_setting *) xsettings_table_find(table, setting->name);
    if (complete && existing != NULL) {
        release(existing);
        *existing = copy;
    } else if (complete && table->count < TABLE_CAPACITY) {
        table->items[table->count++] = copy;
    } else {
        release(&copy);
    }
}
