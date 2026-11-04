#include "color.h"

#include <stdio.h>

#include <glib.h>
#include <pango/pango.h>

enum {
    RGB_COMPONENTS = 3,
    RGBA_COMPONENTS = 4,
    COMPONENT_MAX_16 = 0xFFFF,
    COMPONENT_MAX_8 = 0xFF,
};

typedef struct x11_color {
    const char *name;
    const char *value;
} x11_color;

static const x11_color css_conflicts[] = {
    { "gray", "#bebebe" },
    { "grey", "#bebebe" },
    { "green", "#00ff00" },
    { "maroon", "#b03060" },
    { "purple", "#a020f0" },
};

static const char *x11_spec(const char *spec);
static bool is_named(const char *spec, const char *name);
static bool parse_css_rgb(const char *spec, ui_color *color);
static double component_fraction(uint16_t value);

bool appearance_xpm_color_parse(const char *spec, xpm_rgb *rgb)
{
    PangoColor color;

    if (!pango_color_parse(&color, x11_spec(spec))) {
        return false;
    }
    *rgb = (xpm_rgb) { color.red, color.green, color.blue };
    return true;
}

bool appearance_color_parse(const char *spec, ui_color *color)
{
    xpm_rgb rgb;

    if (parse_css_rgb(spec, color)) {
        return true;
    }
    if (!appearance_xpm_color_parse(spec, &rgb)) {
        return false;
    }
    *color = (ui_color) {
        .red = component_fraction(rgb.red),
        .green = component_fraction(rgb.green),
        .blue = component_fraction(rgb.blue),
        .alpha = 1.0,
    };
    return true;
}

static const char *x11_spec(const char *spec)
{
    for (size_t i = 0; i < G_N_ELEMENTS(css_conflicts); i++) {
        if (is_named(spec, css_conflicts[i].name)) {
            return css_conflicts[i].value;
        }
    }
    return spec;
}

static bool is_named(const char *spec, const char *name)
{
    char *compact = g_strdup(spec);
    char *end = compact;

    for (const char *c = spec; *c != '\0'; c++) {
        if (*c != ' ') {
            *end++ = *c;
        }
    }
    *end = '\0';
    bool equal = g_ascii_strcasecmp(compact, name) == 0;
    g_free(compact);
    return equal;
}

static bool parse_css_rgb(const char *spec, ui_color *color)
{
    double red;
    double green;
    double blue;
    double alpha = 1.0;

    int rgba = sscanf(spec, " rgba ( %lf , %lf , %lf , %lf )", &red, &green, &blue, &alpha);

    if (rgba != RGBA_COMPONENTS
        && sscanf(spec, " rgb ( %lf , %lf , %lf )", &red, &green, &blue) != RGB_COMPONENTS) {
        return false;
    }
    *color = (ui_color) {
        red / COMPONENT_MAX_8, green / COMPONENT_MAX_8, blue / COMPONENT_MAX_8, alpha,
    };
    return true;
}

static double component_fraction(uint16_t value)
{
    return (double) value / COMPONENT_MAX_16;
}
