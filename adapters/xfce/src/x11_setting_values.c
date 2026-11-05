#include "x11_setting_values.h"

#include <string.h>

enum {
    DPI_LOWEST = 50,
    DPI_HIGHEST = 500,
};

static const char *const exported_prefixes[] = {"/Net/", "/Xft/", "/Gtk/", "/Gdk/"};
static const char xft_dpi_property[] = "/Xft/DPI";

static bool is_exported(const char *property);
static bool integer_from(const char *property, const GValue *value, int32_t *integer);

bool x11_setting_from_property(const char *property, const GValue *value, x11_setting *setting)
{
    if (!is_exported(property)) {
        return false;
    }
    setting->name = property + 1;
    if (G_VALUE_HOLDS_STRING(value)) {
        const char *text = g_value_get_string(value);
        setting->type = X11_SETTING_STRING;
        setting->integer = 0;
        setting->string = text != NULL ? text : "";
        return true;
    }
    setting->type = X11_SETTING_INTEGER;
    setting->string = NULL;
    return integer_from(property, value, &setting->integer);
}

static bool is_exported(const char *property)
{
    for (size_t i = 0; i < G_N_ELEMENTS(exported_prefixes); i++) {
        if (g_str_has_prefix(property, exported_prefixes[i])) {
            return true;
        }
    }
    return false;
}

static bool integer_from(const char *property, const GValue *value, int32_t *integer)
{
    if (G_VALUE_HOLDS_BOOLEAN(value)) {
        *integer = g_value_get_boolean(value) ? 1 : 0;
        return true;
    }
    if (!G_VALUE_HOLDS_INT(value)) {
        return false;
    }
    int number = g_value_get_int(value);
    if (strcmp(property, xft_dpi_property) != 0) {
        *integer = number;
        return true;
    }
    *integer = CLAMP(number, DPI_LOWEST, DPI_HIGHEST) * X11_SETTING_XFT_DPI_UNIT;
    return number > 0;
}
