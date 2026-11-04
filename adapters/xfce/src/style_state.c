#include "style_state.h"

#include <glib.h>
#include <stddef.h>
#include <string.h>

struct style_state {
    xfce_style current;
    xfce_style pending;
};

typedef struct color_role {
    const char *name;
    size_t offset;
} color_role;

static const color_role color_roles[] = {
    { "background", offsetof(xfce_style, background) },
    { "foreground", offsetof(xfce_style, foreground) },
    { "selected_background", offsetof(xfce_style, selected_background) },
    { "selected_foreground", offsetof(xfce_style, selected_foreground) },
    { "border", offsetof(xfce_style, border) },
    { "insensitive_foreground", offsetof(xfce_style, insensitive_foreground) },
};

static void begin(style_state *state);
static void commit(style_state *state);
static void set_symbol(xfce_style *style, const char *name, const char *value);
static void set_color(xfce_style *style, const char *role, const char *text);
static bool styles_equal(const xfce_style *a, const xfce_style *b);
static bool symbols_equal(const xfce_style *a, const xfce_style *b);
static bool colors_equal(const xfce_style *a, const xfce_style *b);
static bool color_equal(const xfce_color *a, const xfce_color *b);

style_state *style_state_create(void)
{
    return g_new0(style_state, 1);
}

void style_state_destroy(style_state *state)
{
    g_free(state);
}

xfce_style style_state_current(const style_state *state)
{
    return state->current;
}

bool style_state_pending_differs(const style_state *state)
{
    return !state->current.is_from_gtk || !styles_equal(&state->pending, &state->current);
}

void style_state_apply(style_state *state, const helper_message *message)
{
    switch (message->kind) {
    case HELPER_MESSAGE_STYLE_BEGIN:
        begin(state);
        break;
    case HELPER_MESSAGE_SYMBOL:
        set_symbol(&state->pending, message->name, message->text);
        break;
    case HELPER_MESSAGE_COLOR:
        set_color(&state->pending, message->name, message->text);
        break;
    case HELPER_MESSAGE_FONT:
        g_strlcpy(state->pending.font, message->text, sizeof(state->pending.font));
        break;
    case HELPER_MESSAGE_STYLE_END:
        commit(state);
        break;
    default:
        break;
    }
}

static void begin(style_state *state)
{
    state->pending = (xfce_style) { .is_from_gtk = false };
}

static void commit(style_state *state)
{
    state->current = state->pending;
    state->current.is_from_gtk = true;
}

static void set_symbol(xfce_style *style, const char *name, const char *value)
{
    size_t index = 0;

    while (index < style->symbol_count && strcmp(style->symbols[index].name, name) != 0) {
        index++;
    }
    if (index == XFCE_SYMBOL_MAX) {
        return;
    }
    g_strlcpy(style->symbols[index].name, name, XFCE_SYMBOL_NAME_MAX);
    g_strlcpy(style->symbols[index].value, value, XFCE_SYMBOL_VALUE_MAX);
    style->symbol_count = MAX(style->symbol_count, index + 1);
}

static void set_color(xfce_style *style, const char *role, const char *text)
{
    for (size_t i = 0; i < G_N_ELEMENTS(color_roles); i++) {
        if (strcmp(color_roles[i].name, role) == 0) {
            helper_color_parse(text, (xfce_color *) ((char *) style + color_roles[i].offset));
        }
    }
}

static bool styles_equal(const xfce_style *a, const xfce_style *b)
{
    return symbols_equal(a, b) && colors_equal(a, b) && strcmp(a->font, b->font) == 0;
}

static bool symbols_equal(const xfce_style *a, const xfce_style *b)
{
    if (a->symbol_count != b->symbol_count) {
        return false;
    }
    for (size_t i = 0; i < a->symbol_count; i++) {
        if (strcmp(a->symbols[i].name, b->symbols[i].name) != 0
            || strcmp(a->symbols[i].value, b->symbols[i].value) != 0) {
            return false;
        }
    }
    return true;
}

static bool colors_equal(const xfce_style *a, const xfce_style *b)
{
    for (size_t i = 0; i < G_N_ELEMENTS(color_roles); i++) {
        size_t offset = color_roles[i].offset;
        if (!color_equal((const xfce_color *) ((const char *) a + offset),
                         (const xfce_color *) ((const char *) b + offset))) {
            return false;
        }
    }
    return true;
}

static bool color_equal(const xfce_color *a, const xfce_color *b)
{
    return a->red == b->red && a->green == b->green && a->blue == b->blue && a->alpha == b->alpha;
}
