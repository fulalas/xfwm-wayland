#include "color.h"

#include <errno.h>
#include <math.h>
#include <string.h>

#include <glib.h>
#include <pango/pango.h>

enum {
    RGB_COMPONENTS = 3,
    COMPONENT_MAX_16 = 0xFFFF,
    COMPONENT_MAX_8 = 0xFF,
    PERCENT_MAX = 100,
};

static const char rgb_prefix[] = "rgb";
static const char rgba_prefix[] = "rgba";

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
static bool parse_function_rgb(const char *spec, bool whole, ui_color *color);
static bool parse_channel(const char **p, double *channel);
static bool parse_number(const char **p, double *number);
static bool expect(const char **p, char c);
static const char *skip_spaces(const char *p);
static ui_color opaque(uint16_t red, uint16_t green, uint16_t blue);
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

    if (parse_function_rgb(spec, false, color)) {
        return true;
    }
    if (!appearance_xpm_color_parse(spec, &rgb)) {
        return false;
    }
    *color = opaque(rgb.red, rgb.green, rgb.blue);
    return true;
}

bool appearance_svg_color_parse(const char *spec, ui_color *color)
{
    PangoColor pango;

    if (parse_function_rgb(spec, true, color)) {
        return true;
    }
    if (!pango_color_parse(&pango, spec)) {
        return false;
    }
    *color = opaque(pango.red, pango.green, pango.blue);
    return true;
}

static bool parse_function_rgb(const char *spec, bool whole, ui_color *color)
{
    const char *p = skip_spaces(spec);
    bool has_alpha = strncmp(p, rgba_prefix, strlen(rgba_prefix)) == 0;
    double channels[RGB_COMPONENTS];
    double alpha = 1.0;

    if (!has_alpha && strncmp(p, rgb_prefix, strlen(rgb_prefix)) != 0) {
        return false;
    }
    p = skip_spaces(p + strlen(has_alpha ? rgba_prefix : rgb_prefix));
    if (!expect(&p, '(')) {
        return false;
    }
    for (int i = 0; i < RGB_COMPONENTS; i++) {
        if ((i > 0 && !expect(&p, ',')) || !parse_channel(&p, &channels[i])) {
            return false;
        }
        p = skip_spaces(p);
    }
    if (has_alpha && (!expect(&p, ',') || !parse_number(&p, &alpha))) {
        return false;
    }
    p = skip_spaces(p);
    if (!expect(&p, ')') || (whole && *p != '\0')) {
        return false;
    }
    *color = (ui_color) {
        channels[0], channels[1], channels[2], CLAMP(alpha, 0.0, 1.0),
    };
    return true;
}

static bool expect(const char **p, char c)
{
    if (**p != c) {
        return false;
    }
    *p = skip_spaces(*p + 1);
    return true;
}

static bool parse_channel(const char **p, double *channel)
{
    if (!parse_number(p, channel)) {
        return false;
    }
    const char *after = skip_spaces(*p);
    if (*after == '%') {
        *p = after + 1;
        *channel = CLAMP(*channel / PERCENT_MAX, 0.0, 1.0);
    } else {
        *channel = CLAMP(*channel / COMPONENT_MAX_8, 0.0, 1.0);
    }
    return true;
}

static bool parse_number(const char **p, double *number)
{
    char *end;

    *number = g_ascii_strtod(*p, &end);
    if (errno == ERANGE || end == *p || isinf(*number) || isnan(*number)) {
        return false;
    }
    *p = end;
    return true;
}

static const char *skip_spaces(const char *p)
{
    while (g_ascii_isspace(*p)) {
        p++;
    }
    return p;
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

static ui_color opaque(uint16_t red, uint16_t green, uint16_t blue)
{
    return (ui_color) {
        component_fraction(red), component_fraction(green), component_fraction(blue), 1.0,
    };
}

static double component_fraction(uint16_t value)
{
    return (double) value / COMPONENT_MAX_16;
}
