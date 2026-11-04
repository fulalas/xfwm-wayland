#include "xpm_reader.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

enum {
    WORD_MAX = 1024,
};

struct xpm_reader {
    FILE *file;
    GString *buffer;
};

static bool is_empty(FILE *file);
static bool seek_word(FILE *file, const char *word);
static bool seek_char(FILE *file, int wanted);
static bool skip_comment(FILE *file);
static const char *read_string(xpm_reader *reader);
static bool skip_to_quote(FILE *file);

xpm_reader *appearance_xpm_reader_create(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        return NULL;
    }
    if (is_empty(file)) {
        fclose(file);
        return NULL;
    }
    xpm_reader *reader = g_new0(xpm_reader, 1);
    reader->file = file;
    reader->buffer = g_string_new(NULL);
    return reader;
}

void appearance_xpm_reader_destroy(xpm_reader *reader)
{
    if (reader == NULL) {
        return;
    }
    fclose(reader->file);
    g_string_free(reader->buffer, TRUE);
    g_free(reader);
}

const char *appearance_xpm_reader_header(xpm_reader *reader)
{
    if (!seek_word(reader->file, "XPM") || !seek_char(reader->file, '{')) {
        return NULL;
    }
    return appearance_xpm_reader_colormap(reader);
}

const char *appearance_xpm_reader_colormap(xpm_reader *reader)
{
    seek_char(reader->file, '"');
    fseek(reader->file, -1, SEEK_CUR);
    return read_string(reader);
}

const char *appearance_xpm_reader_row(xpm_reader *reader)
{
    return read_string(reader);
}

static bool is_empty(FILE *file)
{
    bool empty = getc(file) == EOF;

    rewind(file);
    return empty;
}

static bool seek_word(FILE *file, const char *word)
{
    char found[WORD_MAX];

    while (!feof(file)) {
        if (fscanf(file, "%1023s", found) < 0) {
            return false;
        }
        if (strcmp(found, word) == 0) {
            return true;
        }
    }
    return false;
}

static bool seek_char(FILE *file, int wanted)
{
    int c;

    while ((c = getc(file)) != EOF) {
        if (c == wanted) {
            return true;
        }
        if (c == '/' && !skip_comment(file)) {
            return false;
        }
    }
    return false;
}

static bool skip_comment(FILE *file)
{
    int c = getc(file);
    int previous = -1;

    if (c != '*') {
        return c != EOF;
    }
    c = getc(file);
    while (c != EOF && !(previous == '*' && c == '/')) {
        previous = c;
        c = getc(file);
    }
    return c != EOF;
}

static const char *read_string(xpm_reader *reader)
{
    int c;

    g_string_truncate(reader->buffer, 0);
    if (!skip_to_quote(reader->file)) {
        return NULL;
    }
    while ((c = getc(reader->file)) != EOF) {
        if (c == '"') {
            return reader->buffer->str;
        }
        g_string_append_c(reader->buffer, (char) c);
    }
    return NULL;
}

static bool skip_to_quote(FILE *file)
{
    int c;

    do {
        c = getc(file);
    } while (c != EOF && c != '"');
    return c == '"';
}
