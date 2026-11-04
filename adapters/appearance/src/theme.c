#include "theme.h"

#include <stdarg.h>

#include <glib.h>

enum {
    IMAGE_NAME_MAX = 64,
};

struct theme {
    int pixmap_scale;
    cairo_surface_t *side[WM_SIDE_COUNT][WM_FRAME_STATE_COUNT];
    cairo_surface_t *side_stretch[WM_SIDE_COUNT][WM_FRAME_STATE_COUNT];
    cairo_surface_t *corner[WM_CORNER_COUNT][WM_FRAME_STATE_COUNT];
    cairo_surface_t *corner_stretch[WM_CORNER_COUNT][WM_FRAME_STATE_COUNT];
    cairo_surface_t *button[WM_BUTTON_COUNT][WM_IMAGE_COUNT];
    cairo_surface_t *title[WM_TITLE_PART_COUNT][WM_FRAME_STATE_COUNT];
    cairo_surface_t *top[WM_TITLE_PART_COUNT][WM_FRAME_STATE_COUNT];
};

typedef struct pixmap_table {
    cairo_surface_t *const *slots;
    int rows;
    int columns;
} pixmap_table;

static const char *const side_names[WM_SIDE_COUNT] = { "left", "right", "top", "bottom" };
static const char *const corner_names[WM_CORNER_COUNT] = {
    "bottom-left", "bottom-right", "top-left", "top-right",
};
static const char *const button_names[WM_BUTTON_COUNT] = {
    "menu", "stick", "shade", "hide", "maximize", "close",
};
static const char *const image_names[WM_IMAGE_COUNT] = {
    "active", "inactive", "prelight", "inactive-prelight", "pressed", "toggled-active",
    "toggled-inactive", "toggled-prelight", "toggled-inactive-prelight", "toggled-pressed",
};
static const char *const state_names[WM_FRAME_STATE_COUNT] = { "active", "inactive" };

static void load_sides(theme *theme, const theme_image_source *images);
static void load_corners(theme *theme, const theme_image_source *images);
static void load_buttons(theme *theme, const theme_image_source *images);
static void load_titles(theme *theme, const theme_image_source *images);
static cairo_surface_t *load(const theme_image_source *images, const char *format, ...)
    G_GNUC_PRINTF(2, 3);
static pixmap_table table_for(const theme *theme, wm_pixmap_kind kind);
static void destroy_table(pixmap_table table);
static void measure_table(const theme *theme, pixmap_table table, wm_size *sizes);
static wm_size logical_size(const theme *theme, cairo_surface_t *surface);

theme *appearance_theme_create(const theme_source *source)
{
    theme *theme = g_new0(struct theme, 1);

    theme->pixmap_scale = source->pixmap_scale;
    load_sides(theme, &source->images);
    load_corners(theme, &source->images);
    load_buttons(theme, &source->images);
    load_titles(theme, &source->images);
    return theme;
}

void appearance_theme_destroy(theme *theme)
{
    if (theme == NULL) {
        return;
    }
    for (wm_pixmap_kind kind = WM_PIXMAP_SIDE; kind < WM_PIXMAP_APP_ICON; kind++) {
        destroy_table(table_for(theme, kind));
    }
    g_free(theme);
}

cairo_surface_t *appearance_theme_pixmap(const theme *theme, wm_pixmap_ref ref)
{
    if (ref.kind < WM_PIXMAP_SIDE || ref.kind >= WM_PIXMAP_APP_ICON) {
        return NULL;
    }
    pixmap_table table = table_for(theme, ref.kind);
    if (ref.index < 0 || ref.index >= table.rows || ref.image < 0 || ref.image >= table.columns) {
        return NULL;
    }
    return table.slots[ref.index * table.columns + ref.image];
}

int appearance_theme_pixmap_scale(const theme *theme)
{
    return theme->pixmap_scale;
}

wm_theme_metrics appearance_theme_metrics(const theme *theme)
{
    wm_theme_metrics metrics = { 0 };

    measure_table(theme, table_for(theme, WM_PIXMAP_SIDE), &metrics.side[0][0]);
    measure_table(theme, table_for(theme, WM_PIXMAP_SIDE_STRETCH), &metrics.side_stretch[0][0]);
    measure_table(theme, table_for(theme, WM_PIXMAP_CORNER), &metrics.corner[0][0]);
    measure_table(theme, table_for(theme, WM_PIXMAP_CORNER_STRETCH),
                  &metrics.corner_stretch[0][0]);
    measure_table(theme, table_for(theme, WM_PIXMAP_BUTTON), &metrics.button[0][0]);
    measure_table(theme, table_for(theme, WM_PIXMAP_TITLE), &metrics.title[0][0]);
    measure_table(theme, table_for(theme, WM_PIXMAP_TOP), &metrics.top[0][0]);
    return metrics;
}

static void load_sides(theme *theme, const theme_image_source *images)
{
    for (int side = 0; side < WM_SIDE_COUNT; side++) {
        for (int state = 0; side != WM_SIDE_TOP && state < WM_FRAME_STATE_COUNT; state++) {
            theme->side[side][state] = load(images, "%s-%s", side_names[side], state_names[state]);
            theme->side_stretch[side][state] =
                load(images, "%s-%s-stretch", side_names[side], state_names[state]);
        }
    }
    for (int state = 0; state < WM_FRAME_STATE_COUNT; state++) {
        theme->side_stretch[WM_SIDE_TOP][state] =
            load(images, "title-%s-stretch", state_names[state]);
    }
}

static void load_corners(theme *theme, const theme_image_source *images)
{
    for (int corner = 0; corner < WM_CORNER_COUNT; corner++) {
        for (int state = 0; state < WM_FRAME_STATE_COUNT; state++) {
            theme->corner[corner][state] =
                load(images, "%s-%s", corner_names[corner], state_names[state]);
            theme->corner_stretch[corner][state] =
                load(images, "%s-%s-stretch", corner_names[corner], state_names[state]);
        }
    }
}

static void load_buttons(theme *theme, const theme_image_source *images)
{
    for (int button = 0; button < WM_BUTTON_COUNT; button++) {
        for (int image = 0; image < WM_IMAGE_COUNT; image++) {
            theme->button[button][image] =
                load(images, "%s-%s", button_names[button], image_names[image]);
        }
    }
}

static void load_titles(theme *theme, const theme_image_source *images)
{
    for (int part = 0; part < WM_TITLE_PART_COUNT; part++) {
        for (int state = 0; state < WM_FRAME_STATE_COUNT; state++) {
            theme->title[part][state] = load(images, "title-%d-%s", part + 1, state_names[state]);
            theme->top[part][state] = load(images, "top-%d-%s", part + 1, state_names[state]);
        }
    }
}

static cairo_surface_t *load(const theme_image_source *images, const char *format, ...)
{
    char name[IMAGE_NAME_MAX];
    va_list args;

    va_start(args, format);
    g_vsnprintf(name, sizeof name, format, args);
    va_end(args);
    return appearance_theme_image_load(images, name);
}

static pixmap_table table_for(const theme *theme, wm_pixmap_kind kind)
{
    switch (kind) {
    case WM_PIXMAP_SIDE:
        return (pixmap_table) { &theme->side[0][0], WM_SIDE_COUNT, WM_FRAME_STATE_COUNT };
    case WM_PIXMAP_SIDE_STRETCH:
        return (pixmap_table) { &theme->side_stretch[0][0], WM_SIDE_COUNT, WM_FRAME_STATE_COUNT };
    case WM_PIXMAP_CORNER:
        return (pixmap_table) { &theme->corner[0][0], WM_CORNER_COUNT, WM_FRAME_STATE_COUNT };
    case WM_PIXMAP_CORNER_STRETCH:
        return (pixmap_table) { &theme->corner_stretch[0][0], WM_CORNER_COUNT,
                                WM_FRAME_STATE_COUNT };
    case WM_PIXMAP_BUTTON:
        return (pixmap_table) { &theme->button[0][0], WM_BUTTON_COUNT, WM_IMAGE_COUNT };
    case WM_PIXMAP_TITLE:
        return (pixmap_table) { &theme->title[0][0], WM_TITLE_PART_COUNT, WM_FRAME_STATE_COUNT };
    case WM_PIXMAP_TOP:
        return (pixmap_table) { &theme->top[0][0], WM_TITLE_PART_COUNT, WM_FRAME_STATE_COUNT };
    default:
        return (pixmap_table) { NULL, 0, 0 };
    }
}

static void destroy_table(pixmap_table table)
{
    for (int i = 0; i < table.rows * table.columns; i++) {
        if (table.slots[i] != NULL) {
            cairo_surface_destroy(table.slots[i]);
        }
    }
}

static void measure_table(const theme *theme, pixmap_table table, wm_size *sizes)
{
    for (int i = 0; i < table.rows * table.columns; i++) {
        sizes[i] = logical_size(theme, table.slots[i]);
    }
}

static wm_size logical_size(const theme *theme, cairo_surface_t *surface)
{
    if (surface == NULL) {
        return (wm_size) { 0, 0 };
    }
    return (wm_size) {
        cairo_image_surface_get_width(surface) / theme->pixmap_scale,
        cairo_image_surface_get_height(surface) / theme->pixmap_scale,
    };
}
