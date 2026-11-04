#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <wlr/util/log.h>

#include "session_file.h"

enum {
    ID_BYTES = SESSION_ID_LENGTH / 2,
    DIRECTORY_MODE = 0700,
};

static char *session_directory(void);
static char *session_path(const char *id);
static void fill_random(unsigned char *bytes, size_t count);
static void load_lines(char **lines, session_book *book);
static GString *format_book(const session_book *book);
static void evict_if_expired(const char *directory, const char *name, gint64 now);

char *session_file_new_id(void)
{
    unsigned char bytes[ID_BYTES];
    fill_random(bytes, sizeof(bytes));
    GString *id = g_string_sized_new(SESSION_ID_LENGTH);
    for (size_t i = 0; i < sizeof(bytes); i++) {
        g_string_append_printf(id, "%02x", bytes[i]);
    }
    return g_string_free(id, FALSE);
}

bool session_file_load(const char *id, session_book *book)
{
    char *path = session_path(id);
    char *contents = NULL;
    bool loaded = path != NULL && g_file_get_contents(path, &contents, NULL, NULL);
    g_free(path);
    if (!loaded) {
        return false;
    }
    char **lines = g_strsplit(contents, "\n", -1);
    g_free(contents);
    loaded = lines[0] != NULL && g_strcmp0(lines[0], session_file_header) == 0;
    if (loaded) {
        load_lines(lines + 1, book);
    }
    g_strfreev(lines);
    return loaded;
}

bool session_file_save(const char *id, const session_book *book)
{
    char *directory = session_directory();
    char *path = session_path(id);
    GString *text = format_book(book);
    GError *error = NULL;
    bool saved = path != NULL && g_mkdir_with_parents(directory, DIRECTORY_MODE) == 0 &&
                 g_file_set_contents_full(path, text->str, (gssize) text->len,
                                          G_FILE_SET_CONTENTS_CONSISTENT, 0600, &error);
    if (!saved) {
        wlr_log(WLR_ERROR, "cannot save window session %s: %s", id,
                error != NULL ? error->message : g_strerror(errno));
    }
    g_clear_error(&error);
    g_string_free(text, TRUE);
    g_free(path);
    g_free(directory);
    return saved;
}

void session_file_remove(const char *id)
{
    char *path = session_path(id);
    if (path != NULL && g_unlink(path) != 0 && errno != ENOENT) {
        wlr_log(WLR_ERROR, "cannot remove window session %s: %s", id, g_strerror(errno));
    }
    g_free(path);
}

void session_file_evict_expired(void)
{
    char *directory = session_directory();
    GDir *dir = g_dir_open(directory, 0, NULL);
    gint64 now = g_get_real_time() / G_USEC_PER_SEC;
    const char *name = NULL;
    while (dir != NULL && (name = g_dir_read_name(dir)) != NULL) {
        evict_if_expired(directory, name, now);
    }
    if (dir != NULL) {
        g_dir_close(dir);
    }
    g_free(directory);
}

static char *session_directory(void)
{
    return g_build_filename(g_get_user_state_dir(), "xfwm-wayland", "sessions", NULL);
}

static char *session_path(const char *id)
{
    if (!session_id_is_valid(id)) {
        return NULL;
    }
    char *directory = session_directory();
    char *path = g_build_filename(directory, id, NULL);
    g_free(directory);
    return path;
}

static void fill_random(unsigned char *bytes, size_t count)
{
    if (getrandom(bytes, count, 0) == (ssize_t) count) {
        return;
    }
    for (size_t i = 0; i < count; i++) {
        bytes[i] = (unsigned char) g_random_int_range(0, 256);
    }
}

static void load_lines(char **lines, session_book *book)
{
    for (size_t i = 0; lines[i] != NULL; i++) {
        session_record record;
        if (lines[i][0] != '\0' && session_record_parse(lines[i], &record)) {
            session_book_put(book, &record);
            session_record_finish(&record);
        }
    }
}

static GString *format_book(const session_book *book)
{
    GString *text = g_string_new(session_file_header);
    g_string_append_c(text, '\n');
    for (size_t i = 0; i < book->count; i++) {
        char *line = session_record_format(&book->records[i]);
        if (line != NULL) {
            g_string_append(text, line);
            g_string_append_c(text, '\n');
        }
        free(line);
    }
    return text;
}

static void evict_if_expired(const char *directory, const char *name, gint64 now)
{
    GStatBuf info;
    char *path = g_build_filename(directory, name, NULL);
    bool expired = session_id_is_valid(name) && g_stat(path, &info) == 0 &&
                   session_is_expired((int64_t) info.st_mtime, (int64_t) now);
    if (expired) {
        g_unlink(path);
    }
    g_free(path);
}
