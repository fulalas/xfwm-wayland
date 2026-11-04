#ifndef HELPER_UI_STYLE_H
#define HELPER_UI_STYLE_H

#include <gtk/gtk.h>

typedef struct style_query {
    const char *part;
    const char *state;
} style_query;

gboolean ui_style_color(GtkWidget *widget, const style_query *query, GdkRGBA *rgba);
char *ui_style_string(GtkWidget *widget, const style_query *query);

#endif
