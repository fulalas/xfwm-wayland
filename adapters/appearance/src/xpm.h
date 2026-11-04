#ifndef APPEARANCE_XPM_H
#define APPEARANCE_XPM_H

#include <stddef.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct xpm_symbol {
    const char *name;
    const char *value;
} xpm_symbol;

typedef struct xpm_symbols {
    const xpm_symbol *items;
    size_t count;
} xpm_symbols;

char *appearance_xpm_entry_color(const char *entry, const xpm_symbols *symbols);
GdkPixbuf *appearance_xpm_load(const char *path, const xpm_symbols *symbols);

#endif
