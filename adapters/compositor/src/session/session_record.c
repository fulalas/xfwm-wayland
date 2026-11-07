#include "session_record.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wm/desktop.h"

enum {
    LINE_NUMBERS_MAX = 160,
    STATES_TEXT_MAX = 128,
    SECONDS_PER_DAY = 86400,
    EXPIRY_DAYS = 90,
    HEX_BASE = 16,
    ESCAPE_LENGTH = 3,
};

typedef struct state_word {
    uint32_t state;
    const char *word;
} state_word;

static const state_word state_words[] = {
    {WM_STATE_MAXIMIZED_HORZ, "maximized_horz"},
    {WM_STATE_MAXIMIZED_VERT, "maximized_vert"},
    {WM_STATE_FULLSCREEN, "fullscreen"},
    {WM_STATE_MINIMIZED, "minimized"},
    {WM_STATE_SHADED, "shaded"},
    {WM_STATE_STICKY, "sticky"},
    {WM_STATE_ABOVE, "above"},
    {WM_STATE_BELOW, "below"},
};

static const size_t state_word_count = sizeof(state_words) / sizeof(state_words[0]);

const char session_file_header[] = "xfwm-wayland-session 1";

static bool parse_token(const char *token, size_t length, session_record *record);
static void store_number(const char *token, long long value, session_record *record);
static bool has_key(const char *token, const char *key);
static char *encode_name(const char *name);
static char *decode_name(const char *text, size_t length);
static int decode_escape(const char *text, size_t remaining);
static bool is_plain(unsigned char c);
static int hex_value(char c);
static void format_states(uint32_t states, char *text, size_t size);
static uint32_t parse_states(const char *text, size_t length);
static uint32_t state_of_word(const char *word, size_t length);

bool session_id_is_valid(const char *id)
{
    if (id == NULL || strlen(id) != SESSION_ID_LENGTH) {
        return false;
    }
    for (size_t i = 0; i < SESSION_ID_LENGTH; i++) {
        bool lower_hex = (id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f');
        if (!lower_hex) {
            return false;
        }
    }
    return true;
}

bool session_is_expired(int64_t last_used, int64_t now)
{
    return now - last_used > (int64_t) EXPIRY_DAYS * SECONDS_PER_DAY;
}

char *session_record_format(const session_record *r)
{
    char *name = encode_name(r->name);
    char states[STATES_TEXT_MAX];
    format_states(r->states, states, sizeof(states));
    size_t size = (name != NULL ? strlen(name) : 0) + strlen(states) + LINE_NUMBERS_MAX;
    char *line = name != NULL ? malloc(size) : NULL;
    if (line != NULL) {
        snprintf(line, size, "name=%s x=%d y=%d w=%d h=%d workspace=%d states=%s seen=%lld",
                 name, r->geometry.x, r->geometry.y, r->geometry.width, r->geometry.height,
                 r->has_workspace ? r->workspace : -1, states, (long long) r->seen);
    }
    free(name);
    return line;
}

bool session_record_parse(const char *line, session_record *r)
{
    *r = (session_record){.workspace = -1};
    const char *cursor = line;
    while (*cursor != '\0') {
        size_t length = strcspn(cursor, " ");
        if (length > 0 && !parse_token(cursor, length, r)) {
            session_record_finish(r);
            return false;
        }
        cursor += length + (cursor[length] == ' ' ? 1 : 0);
    }
    r->has_workspace = r->workspace >= 0;
    bool valid = r->name != NULL && r->geometry.width > 0 && r->geometry.height > 0;
    if (!valid) {
        session_record_finish(r);
    }
    return valid;
}

void session_record_finish(session_record *r)
{
    free(r->name);
    r->name = NULL;
}

static bool parse_token(const char *token, size_t length, session_record *r)
{
    if (has_key(token, "name=")) {
        free(r->name);
        r->name = decode_name(token + strlen("name="), length - strlen("name="));
        return r->name != NULL;
    }
    if (has_key(token, "states=")) {
        r->states = parse_states(token + strlen("states="), length - strlen("states="));
        return true;
    }
    const char *equals = memchr(token, '=', length);
    char *end = NULL;
    long long value = equals != NULL ? strtoll(equals + 1, &end, 10) : 0;
    if (end != NULL && end != equals + 1 && (*end == ' ' || *end == '\0')) {
        store_number(token, value, r);
    }
    return true;
}

static void store_number(const char *token, long long value, session_record *r)
{
    const char *keys[] = {"x=", "y=", "w=", "h=", "workspace="};
    int *fields[] = {&r->geometry.x, &r->geometry.y, &r->geometry.width, &r->geometry.height,
                     &r->workspace};
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (has_key(token, keys[i])) {
            *fields[i] = (int) value;
        }
    }
    if (has_key(token, "seen=")) {
        r->seen = value;
    }
}

static bool has_key(const char *token, const char *key)
{
    return strncmp(token, key, strlen(key)) == 0;
}

static char *encode_name(const char *name)
{
    size_t length = strlen(name);
    char *text = malloc(length * ESCAPE_LENGTH + 1);
    if (text == NULL) {
        return NULL;
    }
    char *out = text;
    for (size_t i = 0; i < length; i++) {
        unsigned char c = (unsigned char) name[i];
        out += is_plain(c) ? sprintf(out, "%c", c) : sprintf(out, "%%%02x", c);
    }
    *out = '\0';
    return text;
}

static char *decode_name(const char *text, size_t length)
{
    char *name = malloc(length + 1);
    size_t used = 0;
    for (size_t i = 0; name != NULL && i < length; i++) {
        bool escaped = text[i] == '%';
        int byte = escaped ? decode_escape(text + i, length - i) : (unsigned char) text[i];
        if (byte <= 0) {
            free(name);
            return NULL;
        }
        name[used++] = (char) byte;
        i += escaped ? ESCAPE_LENGTH - 1 : 0;
    }
    if (name != NULL) {
        name[used] = '\0';
    }
    return name;
}

static int decode_escape(const char *text, size_t remaining)
{
    if (remaining < ESCAPE_LENGTH) {
        return -1;
    }
    int high = hex_value(text[1]);
    int low = hex_value(text[2]);
    return high < 0 || low < 0 ? -1 : high * HEX_BASE + low;
}

static bool is_plain(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           c == '-' || c == '_' || c == '.' || c == '~' || c == '/' || c == ':';
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static void format_states(uint32_t states, char *text, size_t size)
{
    text[0] = '\0';
    for (size_t i = 0; i < state_word_count; i++) {
        if ((states & state_words[i].state) == 0) {
            continue;
        }
        size_t used = strlen(text);
        snprintf(text + used, size - used, "%s%s", used > 0 ? "," : "", state_words[i].word);
    }
}

static uint32_t parse_states(const char *text, size_t length)
{
    uint32_t states = 0;
    size_t start = 0;
    while (start < length) {
        size_t word = 0;
        while (start + word < length && text[start + word] != ',') {
            word++;
        }
        states |= state_of_word(text + start, word);
        start += word + 1;
    }
    return states;
}

static uint32_t state_of_word(const char *word, size_t length)
{
    for (size_t i = 0; i < state_word_count; i++) {
        const char *known = state_words[i].word;
        if (strlen(known) == length && strncmp(word, known, length) == 0) {
            return state_words[i].state;
        }
    }
    return 0;
}
