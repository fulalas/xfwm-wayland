#include "themerc.h"

#include <stdio.h>
#include <string.h>

#include <glib.h>

enum {
    LINE_MAX_BYTES = 1024,
};

struct themerc {
    GHashTable *values;
};

static void parse_stream(themerc *rc, FILE *file);
static bool line_is_truncated(const char *line, FILE *file);
static void parse_line(themerc *rc, char *line);

themerc *appearance_themerc_create(void)
{
    themerc *rc = g_new0(themerc, 1);

    rc->values = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    return rc;
}

void appearance_themerc_destroy(themerc *rc)
{
    if (rc == NULL) {
        return;
    }
    g_hash_table_destroy(rc->values);
    g_free(rc);
}

bool appearance_themerc_parse_file(themerc *rc, const char *path)
{
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return false;
    }
    parse_stream(rc, file);
    fclose(file);
    return true;
}

void appearance_themerc_set(themerc *rc, const char *key, const char *value)
{
    g_hash_table_replace(rc->values, g_ascii_strdown(key, -1), g_strdup(value));
}

const char *appearance_themerc_get(const themerc *rc, const char *key)
{
    char *folded = g_ascii_strdown(key, -1);
    const char *value = g_hash_table_lookup(rc->values, folded);

    g_free(folded);
    return value;
}

static void parse_stream(themerc *rc, FILE *file)
{
    char line[LINE_MAX_BYTES];
    bool skipping = false;

    while (fgets(line, sizeof line, file) != NULL) {
        if (skipping) {
            skipping = strchr(line, '\n') == NULL;
            continue;
        }
        if (line_is_truncated(line, file)) {
            skipping = true;
            continue;
        }
        parse_line(rc, line);
    }
}

static bool line_is_truncated(const char *line, FILE *file)
{
    return strchr(line, '\n') == NULL && !feof(file);
}

static void parse_line(themerc *rc, char *line)
{
    char *position = NULL;
    const char *key = strtok_r(line, "=", &position);
    const char *value = strtok_r(NULL, "\n", &position);

    if (key != NULL && value != NULL) {
        appearance_themerc_set(rc, key, value);
    }
}
