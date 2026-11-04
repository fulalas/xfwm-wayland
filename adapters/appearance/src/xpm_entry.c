#include "xpm.h"

#include <stdbool.h>
#include <string.h>

#include <glib.h>

enum {
    COLOR_TEXT_MAX = 128,
};

typedef enum entry_key {
    KEY_INVALID = -1,
    KEY_NONE = 0,
    KEY_MONO = 1,
    KEY_GRAY4 = 2,
    KEY_GRAY = 3,
    KEY_COLOR = 4,
    KEY_SYMBOL = 5,
} entry_key;

typedef enum step_result {
    STEP_CONTINUE,
    STEP_DONE,
    STEP_FAIL,
} step_result;

typedef struct entry_state {
    const xpm_symbols *symbols;
    char color[COLOR_TEXT_MAX + 1];
    char current[COLOR_TEXT_MAX + 1];
    entry_key key;
    entry_key current_key;
} entry_state;

typedef struct key_name {
    const char *word;
    entry_key key;
} key_name;

static const key_name key_names[] = {
    { "s", KEY_SYMBOL },
    { "c", KEY_COLOR },
    { "g", KEY_GRAY },
    { "g4", KEY_GRAY4 },
    { "m", KEY_MONO },
};

static const char *next_word(const char *cursor, char word[COLOR_TEXT_MAX + 1]);
static step_result step(entry_state *state, const char *word, bool at_end);
static entry_key word_key(const entry_state *state, const char *word);
static entry_key key_of_word(const char *word);
static step_result append_color_word(entry_state *state, const char *word);
static void commit_color(entry_state *state);
static const char *find_symbol(const xpm_symbols *symbols, const char *name);

char *appearance_xpm_entry_color(const char *entry, const xpm_symbols *symbols)
{
    entry_state state = { .symbols = symbols, .key = KEY_NONE, .current_key = KEY_MONO };
    const char *cursor = entry;
    step_result result = STEP_CONTINUE;

    while (result == STEP_CONTINUE) {
        char word[COLOR_TEXT_MAX + 1];
        cursor = next_word(cursor, word);
        result = step(&state, word, *cursor == '\0');
    }
    if (result == STEP_FAIL || state.current_key <= KEY_MONO) {
        return NULL;
    }
    return g_strdup(state.current);
}

static const char *next_word(const char *cursor, char word[COLOR_TEXT_MAX + 1])
{
    size_t length = 0;

    while (*cursor != '\0' && g_ascii_isspace(*cursor)) {
        cursor++;
    }
    while (*cursor != '\0' && !g_ascii_isspace(*cursor) && length < COLOR_TEXT_MAX) {
        word[length++] = *cursor++;
    }
    word[length] = '\0';
    return cursor;
}

static step_result step(entry_state *state, const char *word, bool at_end)
{
    entry_key key = word_key(state, word);

    if (key == KEY_INVALID) {
        return STEP_FAIL;
    }
    if (key == KEY_NONE) {
        return append_color_word(state, word);
    }
    commit_color(state);
    state->color[0] = '\0';
    state->key = key;
    return at_end ? STEP_DONE : STEP_CONTINUE;
}

static entry_key word_key(const entry_state *state, const char *word)
{
    if (word[0] == '\0') {
        return state->color[0] == '\0' ? KEY_INVALID : KEY_MONO;
    }
    if (state->key > KEY_NONE && state->color[0] == '\0') {
        return KEY_NONE;
    }
    return key_of_word(word);
}

static entry_key key_of_word(const char *word)
{
    for (size_t i = 0; i < G_N_ELEMENTS(key_names); i++) {
        if (strcmp(word, key_names[i].word) == 0) {
            return key_names[i].key;
        }
    }
    return KEY_NONE;
}

static step_result append_color_word(entry_state *state, const char *word)
{
    if (state->key == KEY_NONE) {
        return STEP_FAIL;
    }
    if (state->color[0] != '\0') {
        g_strlcat(state->color, " ", sizeof state->color);
    }
    g_strlcat(state->color, word, sizeof state->color);
    return STEP_CONTINUE;
}

static void commit_color(entry_state *state)
{
    if (state->key == KEY_SYMBOL) {
        const char *value = find_symbol(state->symbols, state->color);
        if (value != NULL) {
            state->current_key = KEY_SYMBOL;
            g_strlcpy(state->current, value, sizeof state->current);
        }
        return;
    }
    if (state->key > state->current_key) {
        state->current_key = state->key;
        g_strlcpy(state->current, state->color, sizeof state->current);
    }
}

static const char *find_symbol(const xpm_symbols *symbols, const char *name)
{
    if (symbols == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < symbols->count; i++) {
        if (g_ascii_strcasecmp(symbols->items[i].name, name) == 0) {
            return symbols->items[i].value;
        }
    }
    return NULL;
}
