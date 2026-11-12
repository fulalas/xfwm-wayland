#include "xcursor_file.h"

#include <glib.h>
#include <stdlib.h>

enum {
    XCURSOR_MAGIC = 0x72756358,
    FILE_HEADER_LENGTH = 16,
    MAX_TOC_ENTRIES = 0x10000,
    MAX_IMAGE_SIDE = 0x7fff,
};

static const uint32_t image_type = 0xfffd0002;
static const char default_theme[] = "default";
static const char default_data_home[] = "~/.local/share";
static const char path_suffix[] =
    "/icons:~/.icons:/usr/share/icons:/usr/share/pixmaps:~/.cursors:/usr/share/cursors/xorg-x11";

typedef struct toc_entry {
    uint32_t type;
    uint32_t subtype;
    uint32_t position;
} toc_entry;

typedef struct toc_table {
    toc_entry *entries;
    uint32_t count;
} toc_table;

static uint32_t le32(const unsigned char *bytes);
static bool read_u32(FILE *file, uint32_t *value);
static bool read_toc(FILE *file, toc_table *toc);
static uint32_t distance(uint32_t a, uint32_t b);
static uint32_t best_size(const toc_table *toc, uint32_t size, size_t *matches);
static void load_larger(FILE *file, const toc_table *toc, uint32_t loaded, int size,
                        xcursor_frames *frames);
static bool read_frames(FILE *file, const toc_table *toc, uint32_t size, size_t count,
                        xcursor_frames *frames);
static bool read_frame(FILE *file, const toc_entry *entry, xcursor_frame *frame);
static bool read_pixels(FILE *file, xcursor_frame *frame);
static void add_theme(char **path, const char *theme, GPtrArray *dirs, GHashTable *visited);
static char **library_path(void);
static char **read_inherits(const char *dir);
static bool load_file(const char *dir, const char *name, const xcursor_request *request,
                      xcursor_frames *frames);

bool xcursor_file_load(FILE *file, const xcursor_request *request, xcursor_frames *frames)
{
    *frames = (xcursor_frames) {0};
    toc_table toc = {0};
    if (file == NULL || request->size < 0 || !read_toc(file, &toc)) {
        return false;
    }
    size_t matches = 0;
    uint32_t best = best_size(&toc, (uint32_t) request->size, &matches);
    bool loaded = best != 0 && read_frames(file, &toc, best, matches, frames);
    if (loaded && request->larger > 0 && frames->frames[0].width != (uint32_t) request->width) {
        load_larger(file, &toc, best, request->larger, frames);
    }
    free(toc.entries);
    return loaded;
}

char **xcursor_search_path(void)
{
    char **path = library_path();
    for (char **entry = path; *entry != NULL; entry++) {
        if ((*entry)[0] == '~') {
            char *expanded = g_build_filename(g_get_home_dir(), *entry + 1, NULL);
            g_free(*entry);
            *entry = expanded;
        }
    }
    return path;
}

char **xcursor_theme_dirs(const char *theme)
{
    char **path = xcursor_search_path();
    GPtrArray *dirs = g_ptr_array_new();
    GHashTable *visited = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    add_theme(path, theme != NULL ? theme : default_theme, dirs, visited);
    g_hash_table_destroy(visited);
    g_strfreev(path);
    g_ptr_array_add(dirs, NULL);
    return (char **) g_ptr_array_free(dirs, FALSE);
}

bool xcursor_theme_load(char *const *dirs, const char *name, const xcursor_request *request,
                        xcursor_frames *frames)
{
    *frames = (xcursor_frames) {0};
    for (; dirs != NULL && *dirs != NULL; dirs++) {
        if (load_file(*dirs, name, request, frames)) {
            return true;
        }
    }
    return false;
}

void xcursor_frames_clear(xcursor_frames *frames)
{
    for (size_t i = 0; i < frames->count; i++) {
        free(frames->frames[i].pixels);
    }
    free(frames->frames);
    *frames = (xcursor_frames) {0};
}

static uint32_t le32(const unsigned char *bytes)
{
    return (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8) | ((uint32_t) bytes[2] << 16)
           | ((uint32_t) bytes[3] << 24);
}

static bool read_u32(FILE *file, uint32_t *value)
{
    unsigned char bytes[4];
    if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
        return false;
    }
    *value = le32(bytes);
    return true;
}

static bool read_toc(FILE *file, toc_table *toc)
{
    uint32_t magic;
    uint32_t header;
    uint32_t version;
    uint32_t count;
    if (!read_u32(file, &magic) || magic != XCURSOR_MAGIC || !read_u32(file, &header)
        || !read_u32(file, &version) || !read_u32(file, &count)) {
        return false;
    }
    uint32_t skip = header - FILE_HEADER_LENGTH;
    if ((skip != 0 && fseek(file, (long) skip, SEEK_CUR) == EOF) || count > MAX_TOC_ENTRIES) {
        return false;
    }
    toc->entries = calloc(count > 0 ? count : 1, sizeof(*toc->entries));
    if (toc->entries == NULL) {
        return false;
    }
    toc->count = count;
    for (uint32_t i = 0; i < count; i++) {
        toc_entry *entry = &toc->entries[i];
        if (!read_u32(file, &entry->type) || !read_u32(file, &entry->subtype)
            || !read_u32(file, &entry->position)) {
            free(toc->entries);
            *toc = (toc_table) {0};
            return false;
        }
    }
    return true;
}

static uint32_t distance(uint32_t a, uint32_t b)
{
    return a > b ? a - b : b - a;
}

static uint32_t best_size(const toc_table *toc, uint32_t size, size_t *matches)
{
    uint32_t best = 0;
    *matches = 0;
    for (uint32_t i = 0; i < toc->count; i++) {
        if (toc->entries[i].type != image_type) {
            continue;
        }
        uint32_t this_size = toc->entries[i].subtype;
        if (best == 0 || distance(this_size, size) < distance(best, size)) {
            best = this_size;
            *matches = 1;
        } else if (this_size == best) {
            (*matches)++;
        }
    }
    return best;
}

static void load_larger(FILE *file, const toc_table *toc, uint32_t loaded, int size,
                        xcursor_frames *frames)
{
    size_t matches = 0;
    uint32_t best = best_size(toc, (uint32_t) size, &matches);
    xcursor_frames larger;
    if (best == loaded) {
        frames->larger = true;
    } else if (read_frames(file, toc, best, matches, &larger)) {
        xcursor_frames_clear(frames);
        *frames = larger;
        frames->larger = true;
    }
}

static bool read_frames(FILE *file, const toc_table *toc, uint32_t size, size_t count,
                        xcursor_frames *frames)
{
    *frames = (xcursor_frames) {calloc(count, sizeof(*frames->frames)), 0, false};
    if (frames->frames == NULL) {
        return false;
    }
    for (uint32_t i = 0; i < toc->count && frames->count < count; i++) {
        const toc_entry *entry = &toc->entries[i];
        if (entry->type != image_type || entry->subtype != size) {
            continue;
        }
        if (!read_frame(file, entry, &frames->frames[frames->count])) {
            break;
        }
        frames->count++;
    }
    if (frames->count != count) {
        xcursor_frames_clear(frames);
        return false;
    }
    return true;
}

static bool read_frame(FILE *file, const toc_entry *entry, xcursor_frame *frame)
{
    uint32_t header_length;
    uint32_t type;
    uint32_t subtype;
    uint32_t version;
    if (fseek(file, (long) entry->position, SEEK_SET) == EOF || !read_u32(file, &header_length)
        || !read_u32(file, &type) || !read_u32(file, &subtype) || !read_u32(file, &version)) {
        return false;
    }
    if (type != entry->type || subtype != entry->subtype) {
        return false;
    }
    if (!read_u32(file, &frame->width) || !read_u32(file, &frame->height)
        || !read_u32(file, &frame->hotspot_x) || !read_u32(file, &frame->hotspot_y)
        || !read_u32(file, &frame->delay)) {
        return false;
    }
    if (frame->width > MAX_IMAGE_SIDE || frame->height > MAX_IMAGE_SIDE || frame->width == 0
        || frame->height == 0 || frame->hotspot_x > frame->width
        || frame->hotspot_y > frame->height) {
        return false;
    }
    return read_pixels(file, frame);
}

static bool read_pixels(FILE *file, xcursor_frame *frame)
{
    size_t count = (size_t) frame->width * frame->height;
    frame->pixels = malloc(count * sizeof(uint32_t));
    if (frame->pixels == NULL) {
        return false;
    }
    if (fread(frame->pixels, sizeof(uint32_t), count, file) != count) {
        free(frame->pixels);
        frame->pixels = NULL;
        return false;
    }
    const unsigned char *bytes = (const unsigned char *) frame->pixels;
    for (size_t i = 0; i < count; i++) {
        frame->pixels[i] = le32(bytes + i * sizeof(uint32_t));
    }
    return true;
}

static void add_theme(char **path, const char *theme, GPtrArray *dirs, GHashTable *visited)
{
    g_hash_table_add(visited, g_strdup(theme));
    char **inherits = NULL;
    for (char **entry = path; *entry != NULL; entry++) {
        char *base = g_build_filename(*entry, theme, NULL);
        char *cursors = g_build_filename(base, "cursors", NULL);
        if (g_file_test(cursors, G_FILE_TEST_IS_DIR)) {
            g_ptr_array_add(dirs, cursors);
        } else {
            g_free(cursors);
        }
        if (inherits == NULL) {
            inherits = read_inherits(base);
        }
        g_free(base);
    }
    for (char **name = inherits; name != NULL && *name != NULL; name++) {
        if (**name != '\0' && !g_hash_table_contains(visited, *name)) {
            add_theme(path, *name, dirs, visited);
        }
    }
    g_strfreev(inherits);
}

static char **library_path(void)
{
    const char *custom = getenv("XCURSOR_PATH");
    if (custom != NULL) {
        return g_strsplit(custom, ":", -1);
    }
    const char *data = getenv("XDG_DATA_HOME");
    if (data == NULL || data[0] != '/') {
        data = default_data_home;
    }
    char *joined = g_strconcat(data, path_suffix, NULL);
    char **path = g_strsplit(joined, ":", -1);
    g_free(joined);
    return path;
}

static char **read_inherits(const char *dir)
{
    char *path = g_build_filename(dir, "index.theme", NULL);
    GKeyFile *file = g_key_file_new();
    char *value = g_key_file_load_from_file(file, path, G_KEY_FILE_NONE, NULL)
                      ? g_key_file_get_string(file, "Icon Theme", "Inherits", NULL)
                      : NULL;
    char **inherits = value != NULL ? g_strsplit_set(value, " \t\n;,", -1) : NULL;
    g_free(value);
    g_key_file_free(file);
    g_free(path);
    return inherits;
}

static bool load_file(const char *dir, const char *name, const xcursor_request *request,
                      xcursor_frames *frames)
{
    char *path = g_build_filename(dir, name, NULL);
    FILE *file = fopen(path, "r");
    g_free(path);
    if (file == NULL) {
        return false;
    }
    bool loaded = xcursor_file_load(file, request, frames);
    fclose(file);
    return loaded;
}
