#include "style_report.h"

#include "protocol_io.h"
#include "ui_style.h"

enum {
    COLOR_MAX = 255,
};

typedef struct symbol_source {
    const char *name;
    style_query query;
} symbol_source;

static const symbol_source symbol_sources[] = {
    { "active_text_color", { "fg", "selected" } },
    { "inactive_text_color", { "fg", "insensitive" } },
    { "active_text_shadow_color", { "dark", "selected" } },
    { "inactive_text_shadow_color", { "dark", "insensitive" } },
    { "active_border_color", { "fg", "normal" } },
    { "inactive_border_color", { "fg", "normal" } },
    { "active_color_1", { "bg", "selected" } },
    { "active_hilight_1", { "light", "selected" } },
    { "active_shadow_1", { "dark", "selected" } },
    { "active_mid_1", { "mid", "selected" } },
    { "active_text_color_2", { "fg", "normal" } },
    { "active_color_2", { "bg", "normal" } },
    { "active_hilight_2", { "light", "normal" } },
    { "active_shadow_2", { "dark", "normal" } },
    { "active_mid_2", { "mid", "normal" } },
    { "inactive_color_1", { "bg", "insensitive" } },
    { "inactive_hilight_1", { "light", "insensitive" } },
    { "inactive_shadow_1", { "dark", "insensitive" } },
    { "inactive_mid_1", { "mid", "insensitive" } },
    { "inactive_text_color_2", { "fg", "insensitive" } },
    { "inactive_color_2", { "bg", "normal" } },
    { "inactive_hilight_2", { "light", "normal" } },
    { "inactive_shadow_2", { "dark", "normal" } },
    { "inactive_mid_2", { "mid", "normal" } },
};

typedef struct ui_color_source {
    const char *role;
    const char *named_color;
    style_query query;
} ui_color_source;

static const ui_color_source ui_color_sources[] = {
    { "background", "theme_bg_color", { "bg", "normal" } },
    { "foreground", "theme_fg_color", { "fg", "normal" } },
    { "selected_background", "theme_selected_bg_color", { "bg", "selected" } },
    { "selected_foreground", "theme_selected_fg_color", { "fg", "selected" } },
    { "border", "borders", { "dark", "normal" } },
    { "insensitive_foreground", "insensitive_fg_color", { "fg", "insensitive" } },
};

static void send_symbols(GtkWidget *widget);
static void send_ui_colors(GtkWidget *widget);
static void send_font(void);
static GdkRGBA ui_color(GtkWidget *widget, const ui_color_source *source);
static char *rgba_text(const GdkRGBA *color);
static int channel_byte(double channel);

void style_report_send(GtkWidget *widget)
{
    protocol_write("style-begin", NULL, NULL);
    send_symbols(widget);
    send_ui_colors(widget);
    send_font();
    protocol_write("style-end", NULL, NULL);
}

static void send_symbols(GtkWidget *widget)
{
    for (size_t i = 0; i < G_N_ELEMENTS(symbol_sources); i++) {
        const symbol_source *source = &symbol_sources[i];
        char *value = ui_style_string(widget, &source->query);
        protocol_write("symbol", source->name, value);
        g_free(value);
    }
}

static void send_ui_colors(GtkWidget *widget)
{
    for (size_t i = 0; i < G_N_ELEMENTS(ui_color_sources); i++) {
        GdkRGBA color = ui_color(widget, &ui_color_sources[i]);
        char *text = rgba_text(&color);
        protocol_write("color", ui_color_sources[i].role, text);
        g_free(text);
    }
}

static void send_font(void)
{
    char *font = NULL;

    g_object_get(gtk_settings_get_default(), "gtk-font-name", &font, NULL);
    protocol_write("font", font ? font : "", NULL);
    g_free(font);
}

static GdkRGBA ui_color(GtkWidget *widget, const ui_color_source *source)
{
    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    GdkRGBA color = { 0 };

    if (!gtk_style_context_lookup_color(context, source->named_color, &color)) {
        ui_style_color(widget, &source->query, &color);
    }
    return color;
}

static char *rgba_text(const GdkRGBA *color)
{
    return g_strdup_printf("#%02x%02x%02x%02x", channel_byte(color->red),
                           channel_byte(color->green), channel_byte(color->blue),
                           channel_byte(color->alpha));
}

static int channel_byte(double channel)
{
    return (int) (CLAMP(channel, 0.0, 1.0) * COLOR_MAX + 0.5);
}
