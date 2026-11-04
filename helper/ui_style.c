#include "ui_style.h"

#include <string.h>

typedef enum style_part {
    PART_FG,
    PART_BG,
    PART_LIGHT,
    PART_DARK,
    PART_MID,
    PART_INVALID,
} style_part;

enum {
    HUE_SEXTANT = 60,
    HUE_HALF = 180,
    HUE_TWO_THIRDS = 240,
    HUE_THIRD = 120,
    HUE_FULL = 360,
    HUE_GREEN_OFFSET = 2,
    HUE_BLUE_OFFSET = 4,
    COLOR_MAX = 0xff,
};

static const double lightness_mult = 1.3;
static const double darkness_mult = 0.7;

static const char *const part_names[] = { "fg", "bg", "light", "dark", "mid" };

typedef struct state_name {
    const char *name;
    GtkStateFlags flags;
} state_name;

static const state_name state_names[] = {
    { "normal", GTK_STATE_FLAG_NORMAL },
    { "active", GTK_STATE_FLAG_ACTIVE },
    { "prelight", GTK_STATE_FLAG_PRELIGHT },
    { "selected", GTK_STATE_FLAG_SELECTED },
    { "insensitive", GTK_STATE_FLAG_INSENSITIVE },
};

static style_part part_from_name(const char *name);
static gboolean state_from_name(const char *name, GtkStateFlags *flags);
static GdkRGBA context_color(GtkWidget *widget, style_part part, GtkStateFlags flags);
static void apply_part(style_part part, GdkRGBA *rgba);
static void rgba_shade(GdkRGBA *color, double value);
static void rgb_to_hls(double *r, double *g, double *b);
static double hue_of(double red, double green, double blue);
static void hls_to_rgb(double *h, double *l, double *s);
static double hue_component(double m1, double m2, double hue);

gboolean ui_style_color(GtkWidget *widget, const style_query *query, GdkRGBA *rgba)
{
    style_part part = part_from_name(query->part);
    GtkStateFlags flags;

    if (part == PART_INVALID || !state_from_name(query->state, &flags)) {
        return FALSE;
    }
    *rgba = context_color(widget, part, flags);
    apply_part(part, rgba);
    return TRUE;
}

char *ui_style_string(GtkWidget *widget, const style_query *query)
{
    style_query bg_query = { .part = "bg", .state = query->state };
    GdkRGBA color = { 0 };
    GdkRGBA bg = { 0 };
    int red;
    int green;
    int blue;

    if (ui_style_color(widget, query, &color) && color.alpha < 1 &&
        g_strcmp0(query->part, "bg") && ui_style_color(widget, &bg_query, &bg)) {
        color.red = color.red * color.alpha + bg.red * (1 - color.alpha);
        color.green = color.green * color.alpha + bg.green * (1 - color.alpha);
        color.blue = color.blue * color.alpha + bg.blue * (1 - color.alpha);
    }
    red = color.red * COLOR_MAX;
    green = color.green * COLOR_MAX;
    blue = color.blue * COLOR_MAX;
    return g_strdup_printf("#%02x%02x%02x%02x%02x%02x", red, red, green, green, blue, blue);
}

static style_part part_from_name(const char *name)
{
    for (size_t i = 0; i < G_N_ELEMENTS(part_names); i++) {
        if (strcmp(part_names[i], name) == 0) {
            return (style_part) i;
        }
    }
    return PART_INVALID;
}

static gboolean state_from_name(const char *name, GtkStateFlags *flags)
{
    for (size_t i = 0; i < G_N_ELEMENTS(state_names); i++) {
        if (strcmp(state_names[i].name, name) == 0) {
            *flags = state_names[i].flags;
            return TRUE;
        }
    }
    return FALSE;
}

static GdkRGBA context_color(GtkWidget *widget, style_part part, GtkStateFlags flags)
{
    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    const char *property =
        part == PART_FG ? GTK_STYLE_PROPERTY_COLOR : GTK_STYLE_PROPERTY_BACKGROUND_COLOR;
    GdkRGBA *result = NULL;
    GdkRGBA color;

    gtk_style_context_save(context);
    gtk_style_context_add_class(context, "gtkstyle-fallback");
    gtk_style_context_get(context, flags, property, &result, NULL);
    gtk_style_context_restore(context);
    color = *result;
    gdk_rgba_free(result);
    return color;
}

static void apply_part(style_part part, GdkRGBA *rgba)
{
    GdkRGBA dark = *rgba;

    if (part == PART_LIGHT) {
        rgba_shade(rgba, lightness_mult);
    } else if (part == PART_DARK) {
        rgba_shade(rgba, darkness_mult);
    } else if (part == PART_MID) {
        rgba_shade(rgba, lightness_mult);
        rgba_shade(&dark, darkness_mult);
        rgba->red = (rgba->red + dark.red) / 2;
        rgba->green = (rgba->green + dark.green) / 2;
        rgba->blue = (rgba->blue + dark.blue) / 2;
    }
}

static void rgba_shade(GdkRGBA *color, double value)
{
    rgb_to_hls(&color->red, &color->green, &color->blue);
    color->green = MAX(MIN(color->green * value, 1), 0);
    color->blue = MAX(MIN(color->blue * value, 1), 0);
    hls_to_rgb(&color->red, &color->green, &color->blue);
}

static void rgb_to_hls(double *r, double *g, double *b)
{
    double max = MAX(*r, MAX(*g, *b));
    double min = MIN(*r, MIN(*g, *b));
    double l = (max + min) / 2;
    double s = 0;
    double h = 0;

    if (max != min) {
        s = l <= 0.5 ? (max - min) / (max + min) : (max - min) / (2 - max - min);
        h = hue_of(*r, *g, *b);
    }
    *r = h;
    *g = l;
    *b = s;
}

static double hue_of(double red, double green, double blue)
{
    double max = MAX(red, MAX(green, blue));
    double delta = max - MIN(red, MIN(green, blue));
    double h = 0;

    if (red == max) {
        h = (green - blue) / delta;
    } else if (green == max) {
        h = HUE_GREEN_OFFSET + (blue - red) / delta;
    } else {
        h = HUE_BLUE_OFFSET + (red - green) / delta;
    }
    h *= HUE_SEXTANT;
    return h < 0.0 ? h + HUE_FULL : h;
}

static void hls_to_rgb(double *h, double *l, double *s)
{
    double lightness = *l;
    double saturation = *s;
    double m2 = lightness <= 0.5 ? lightness * (1 + saturation)
                                 : lightness + saturation - lightness * saturation;
    double m1 = 2 * lightness - m2;
    double hue = *h;

    if (saturation == 0) {
        *h = lightness;
        *l = lightness;
        *s = lightness;
        return;
    }
    *h = hue_component(m1, m2, hue + HUE_THIRD);
    *l = hue_component(m1, m2, hue);
    *s = hue_component(m1, m2, hue - HUE_THIRD);
}

static double hue_component(double m1, double m2, double hue)
{
    while (hue > HUE_FULL) {
        hue -= HUE_FULL;
    }
    while (hue < 0) {
        hue += HUE_FULL;
    }
    if (hue < HUE_SEXTANT) {
        return m1 + (m2 - m1) * hue / HUE_SEXTANT;
    }
    if (hue < HUE_HALF) {
        return m2;
    }
    if (hue < HUE_TWO_THIRDS) {
        return m1 + (m2 - m1) * (HUE_TWO_THIRDS - hue) / HUE_SEXTANT;
    }
    return m1;
}
