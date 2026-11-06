#include "appearance/appearance.h"

#include <glib.h>

#include "appearance_internal.h"
#include "color.h"
#include "color_symbols.h"
#include "request_memo.h"
#include "theme.h"
#include "theme_dir.h"
#include "themerc.h"

static const char themerc_file[] = "themerc";
static const char title_font_option[] = "title_font";

struct appearance {
    theme *theme;
    themerc *rc;
    color_symbol_values symbols;
    ui_style style;
    title_style title;
    text_font ui_font;
    request_memo loaded;
};

typedef struct loaded_theme {
    theme *theme;
    themerc *rc;
} loaded_theme;

static void apply_style(appearance *appearance, const theme_request *request);
static void apply_title_font(appearance *appearance, const char *theme_font,
                             const char *request_font);
static bool load_theme(appearance *appearance, const theme_request *request,
                       loaded_theme *loaded);
static void keep_previous_theme(appearance *appearance, const theme_request *request);
static themerc *parse_themerc(const char *dir);
static void apply_theme(appearance *appearance, loaded_theme *loaded);
static void update_title_colors(appearance *appearance);
static ui_color symbol_color(const appearance *appearance, color_symbol_id id,
                             const ui_color *fallback);
static void replace_font(const appearance *appearance, text_font *font,
                         const char *description);

appearance *appearance_create(void)
{
    appearance *appearance = g_new0(struct appearance, 1);

    appearance->style = appearance_fallback_style();
    replace_font(appearance, &appearance->ui_font, appearance->style.font);
    replace_font(appearance, &appearance->title.font, appearance->style.font);
    appearance_color_symbols_resolve(&appearance->symbols, &appearance->style, NULL);
    update_title_colors(appearance);
    return appearance;
}

void appearance_destroy(appearance *appearance)
{
    if (appearance == NULL) {
        return;
    }
    appearance_theme_destroy(appearance->theme);
    appearance_themerc_destroy(appearance->rc);
    appearance_text_font_destroy(&appearance->ui_font);
    appearance_text_font_destroy(&appearance->title.font);
    appearance_request_memo_clear(&appearance->loaded);
    g_free(appearance);
}

bool appearance_load(appearance *appearance, const theme_request *request)
{
    loaded_theme loaded;

    if (appearance_is_loaded(appearance, request)) {
        return true;
    }
    apply_style(appearance, request);
    if (!load_theme(appearance, request, &loaded)) {
        keep_previous_theme(appearance, request);
        return false;
    }
    apply_theme(appearance, &loaded);
    apply_title_font(appearance, appearance_theme_option(appearance, title_font_option),
                     request->title_font);
    appearance_request_memo_store(&appearance->loaded, request);
    return true;
}

bool appearance_is_loaded(const appearance *appearance, const theme_request *request)
{
    return appearance_request_memo_matches(&appearance->loaded, request);
}

const char *appearance_theme_option(const appearance *appearance, const char *key)
{
    if (appearance->rc == NULL || key == NULL) {
        return NULL;
    }
    return appearance_themerc_get(appearance->rc, key);
}

wm_theme_metrics appearance_metrics(const appearance *appearance)
{
    if (appearance->theme == NULL) {
        return (wm_theme_metrics) { 0 };
    }
    return appearance_theme_metrics(appearance->theme);
}

frame_painter appearance_frame_painter(const appearance *appearance)
{
    return (frame_painter) { appearance->theme, &appearance->title };
}

ui_text appearance_ui_text(const appearance *appearance)
{
    return (ui_text) { &appearance->style, &appearance->ui_font };
}

const title_style *appearance_title_style(const appearance *appearance)
{
    return &appearance->title;
}

static void apply_style(appearance *appearance, const theme_request *request)
{
    appearance->style = request->style != NULL ? *request->style : appearance_fallback_style();
    replace_font(appearance, &appearance->ui_font, appearance->style.font);
}

static void apply_title_font(appearance *appearance, const char *theme_font,
                             const char *request_font)
{
    const char *wanted = theme_font != NULL ? theme_font : request_font;

    if (wanted == NULL || wanted[0] == '\0') {
        wanted = appearance->style.font;
    }
    replace_font(appearance, &appearance->title.font, wanted);
}

static bool load_theme(appearance *appearance, const theme_request *request,
                       loaded_theme *loaded)
{
    theme_location location = appearance_theme_locate(request->name, request->max_output_scale);
    themerc *rc = parse_themerc(location.dir);

    if (rc == NULL) {
        appearance_theme_location_clear(&location);
        return false;
    }
    appearance_color_symbols_resolve(&appearance->symbols, &appearance->style, rc);
    xpm_symbols symbols = appearance_color_symbols_view(&appearance->symbols);
    theme_source source = { { location.dir, &symbols }, location.pixmap_scale };
    *loaded = (loaded_theme) { appearance_theme_create(&source), rc };
    appearance_theme_location_clear(&location);
    return true;
}

static void keep_previous_theme(appearance *appearance, const theme_request *request)
{
    appearance_request_memo_clear(&appearance->loaded);
    apply_title_font(appearance, NULL, request->title_font);
    appearance_color_symbols_resolve(&appearance->symbols, &appearance->style, appearance->rc);
    update_title_colors(appearance);
}

static themerc *parse_themerc(const char *dir)
{
    char *path = g_build_filename(dir, themerc_file, NULL);
    themerc *rc = appearance_themerc_create();

    if (!appearance_themerc_parse_file(rc, path)) {
        g_warning("Cannot read theme file %s", path);
        appearance_themerc_destroy(rc);
        rc = NULL;
    }
    g_free(path);
    return rc;
}

static void apply_theme(appearance *appearance, loaded_theme *loaded)
{
    appearance_theme_destroy(appearance->theme);
    appearance_themerc_destroy(appearance->rc);
    appearance->theme = loaded->theme;
    appearance->rc = loaded->rc;
    update_title_colors(appearance);
}

static void update_title_colors(appearance *appearance)
{
    const ui_color *text = &appearance->style.foreground;
    const ui_color *shadow = &appearance->style.background;
    title_style *title = &appearance->title;

    title->text[WM_FRAME_ACTIVE] = symbol_color(appearance, SYMBOL_ACTIVE_TEXT, text);
    title->text[WM_FRAME_INACTIVE] = symbol_color(appearance, SYMBOL_INACTIVE_TEXT, text);
    title->shadow[WM_FRAME_ACTIVE] = symbol_color(appearance, SYMBOL_ACTIVE_TEXT_SHADOW, shadow);
    title->shadow[WM_FRAME_INACTIVE] =
        symbol_color(appearance, SYMBOL_INACTIVE_TEXT_SHADOW, shadow);
}

static ui_color symbol_color(const appearance *appearance, color_symbol_id id,
                             const ui_color *fallback)
{
    ui_color color;

    if (!appearance_color_parse(appearance->symbols.value[id], &color)) {
        return *fallback;
    }
    return color;
}

static void replace_font(const appearance *appearance, text_font *font,
                         const char *description)
{
    text_font_request request = { description, appearance->style.font_dpi };

    appearance_text_font_destroy(font);
    *font = appearance_text_font_create(&request);
}
