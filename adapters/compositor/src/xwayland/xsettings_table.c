#include "xsettings_table.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "wm/geometry.h"

enum {
    TABLE_CAPACITY = 128,
    XWAYLAND_DPI = X11_SETTING_UNSCALED_DPI * X11_SETTING_XFT_DPI_UNIT,
};

static const double WHOLE_SCALE_SLACK = 1e-6;
static const char window_scaling_factor[] = "Gdk/WindowScalingFactor";
static const char unscaled_dpi[] = "Gdk/UnscaledDPI";
const char xsettings_cursor_size[] = "Gtk/CursorThemeSize";
static const char *const ignored_names[] = {window_scaling_factor, unscaled_dpi,
                                            X11_SETTING_XFT_DPI};

struct xsettings_table {
    x11_setting items[TABLE_CAPACITY];
    size_t count;
    int window_scale;
    int cursor_size;
};

static bool is_ignored(const char *name);
static void release(x11_setting *setting);
static x11_setting copy_of(const x11_setting *setting);
static void store(xsettings_table *table, const x11_setting *setting);
static void store_integer(xsettings_table *table, const char *name, int value);

xsettings_table *xsettings_table_create(void)
{
    xsettings_table *table = calloc(1, sizeof(*table));
    if (table != NULL) {
        xsettings_table_reset(table, 1.0);
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

void xsettings_table_reset(xsettings_table *table, double scale)
{
    for (size_t i = 0; i < table->count; i++) {
        release(&table->items[i]);
    }
    table->count = 0;
    int window_scale = wm_max((int) floor(scale + WHOLE_SCALE_SLACK), 1);
    table->window_scale = window_scale;
    table->cursor_size = 0;
    store_integer(table, window_scaling_factor, window_scale);
    store_integer(table, X11_SETTING_XFT_DPI, (int) lround(XWAYLAND_DPI * scale));
    if (scale != 1.0) {
        store_integer(table, unscaled_dpi, (int) lround(XWAYLAND_DPI * scale / window_scale));
    }
}

void xsettings_table_add(xsettings_table *table, const x11_setting *setting)
{
    if (setting->name == NULL || is_ignored(setting->name)) {
        return;
    }
    if (setting->type == X11_SETTING_STRING && setting->string == NULL) {
        return;
    }
    if (setting->type == X11_SETTING_INTEGER && strcmp(setting->name, xsettings_cursor_size) == 0) {
        table->cursor_size = setting->integer;
        int per_window_scale = (int) lround((double) setting->integer / table->window_scale);
        store_integer(table, xsettings_cursor_size, per_window_scale);
        return;
    }
    store(table, setting);
}

int xsettings_table_cursor_size(const xsettings_table *table)
{
    return table->cursor_size;
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

static void store_integer(xsettings_table *table, const char *name, int value)
{
    x11_setting setting = {name, X11_SETTING_INTEGER, value, NULL};
    store(table, &setting);
}
