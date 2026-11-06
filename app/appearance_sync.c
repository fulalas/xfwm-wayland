#include <string.h>

#include "app.h"

static bool theme_is_current(app *app);
static void apply_appearance(app *app);
static ui_style current_style(app *app);
static theme_request request_for(app *app, const ui_style *style);
static wm_config current_config(app *app);
static void push_config(app *app);
static const char *lookup_theme_option(void *self, const char *key);
static ui_style style_from_xfce(const xfce_style *source);
static void copy_symbols(const xfce_style *source, ui_style *target);
static const char *symbol_value(const xfce_style *source, const char *name);
static ui_color color_from_xfce(xfce_color color);

void app_load_appearance(app *app)
{
    ui_style style = current_style(app);
    theme_request request = request_for(app, &style);

    if (!appearance_load(app->appearance, &request)) {
        g_warning("Cannot load the xfwm4 theme \"%s\"", request.name);
    }
    app->config = current_config(app);
}

void app_push_appearance(app *app)
{
    compositor_appearance_changed(app->compositor);
    push_config(app);
}

void app_reload_appearance(app *app)
{
    if (!theme_is_current(app)) {
        apply_appearance(app);
    }
}

void app_reload_theme(app *app)
{
    if (theme_is_current(app)) {
        app_reload_config(app);
        return;
    }
    apply_appearance(app);
}

void app_reload_config(app *app)
{
    wm_config config = current_config(app);

    if (memcmp(&config, &app->config, sizeof(config)) == 0) {
        return;
    }
    app->config = config;
    push_config(app);
}

void app_reload_keymap(app *app)
{
    wm_keymap keymap = xfce_environment_keymap(app->xfce);

    wm_desktop_set_keymap(app->desktop, &keymap);
}

static bool theme_is_current(app *app)
{
    ui_style style = current_style(app);
    theme_request request = request_for(app, &style);

    return appearance_is_loaded(app->appearance, &request);
}

static void apply_appearance(app *app)
{
    app_load_appearance(app);
    app_push_appearance(app);
}

static ui_style current_style(app *app)
{
    xfce_style xfce = xfce_environment_style(app->xfce);
    ui_style style = style_from_xfce(&xfce);
    int dpi = xfce_environment_font_dpi(app->xfce);

    style.font_dpi = dpi > 0 ? dpi / app->max_output_scale : 0.0;
    return style;
}

static theme_request request_for(app *app, const ui_style *style)
{
    return (theme_request) {
        .name = xfce_environment_theme_name(app->xfce),
        .title_font = xfce_environment_title_font(app->xfce),
        .max_output_scale = app->max_output_scale,
        .style = style,
    };
}

static wm_config current_config(app *app)
{
    xfce_setting_source theme = { .self = app->appearance, .lookup = lookup_theme_option };

    return xfce_environment_config(app->xfce, &theme);
}

static void push_config(app *app)
{
    wm_theme_metrics metrics = appearance_metrics(app->appearance);

    compositor_set_config(app->compositor, &app->config);
    wm_desktop_set_appearance(app->desktop, &metrics, &app->config);
}

static const char *lookup_theme_option(void *self, const char *key)
{
    return appearance_theme_option(self, key);
}

static ui_style style_from_xfce(const xfce_style *source)
{
    ui_style style = appearance_fallback_style();

    if (!source->is_from_gtk) {
        return style;
    }
    copy_symbols(source, &style);
    style.background = color_from_xfce(source->background);
    style.foreground = color_from_xfce(source->foreground);
    style.selected_background = color_from_xfce(source->selected_background);
    style.selected_foreground = color_from_xfce(source->selected_foreground);
    style.border = color_from_xfce(source->border);
    style.insensitive_foreground = color_from_xfce(source->insensitive_foreground);
    g_strlcpy(style.font, source->font, sizeof(style.font));
    return style;
}

static void copy_symbols(const xfce_style *source, ui_style *target)
{
    for (int i = 0; i < APPEARANCE_COLOR_SYMBOL_COUNT; i++) {
        const char *value = symbol_value(source, appearance_color_symbol_name(i));
        if (value != NULL) {
            g_strlcpy(target->color_symbols[i], value, sizeof(target->color_symbols[i]));
        }
    }
}

static const char *symbol_value(const xfce_style *source, const char *name)
{
    const char *value = NULL;
    for (size_t j = 0; j < source->symbol_count; j++) {
        if (strcmp(source->symbols[j].name, name) == 0) {
            value = source->symbols[j].value;
        }
    }
    return value;
}

static ui_color color_from_xfce(xfce_color color)
{
    return (ui_color) { color.red, color.green, color.blue, color.alpha };
}
