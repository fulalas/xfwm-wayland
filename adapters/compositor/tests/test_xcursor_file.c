#include <ftw.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "cursor/xcursor_file.h"

enum {
    IMAGE_TYPE = 0xfffd0002,
    COMMENT_TYPE = 0xfffe0001,
    MAX_WORDS = 256,
};

typedef struct writer {
    uint32_t words[MAX_WORDS];
    size_t count;
} writer;

typedef struct image_spec {
    uint32_t size;
    uint32_t width;
    uint32_t delay;
    uint32_t color;
} image_spec;

static void put(writer *w, uint32_t value)
{
    w->words[w->count++] = value;
}

static size_t build_file(const image_spec *images, size_t count, unsigned char *out)
{
    writer w = {0};
    put(&w, 0x72756358);
    put(&w, 16);
    put(&w, 0x10000);
    put(&w, (uint32_t) count + 1);
    size_t toc = w.count;
    w.count += (count + 1) * 3;
    w.words[toc] = COMMENT_TYPE;
    w.words[toc + 1] = 1;
    w.words[toc + 2] = 0;
    for (size_t i = 0; i < count; i++) {
        const image_spec *spec = &images[i];
        size_t entry = toc + (i + 1) * 3;
        w.words[entry] = IMAGE_TYPE;
        w.words[entry + 1] = spec->size;
        w.words[entry + 2] = (uint32_t) (w.count * 4);
        put(&w, 36);
        put(&w, IMAGE_TYPE);
        put(&w, spec->size);
        put(&w, 1);
        put(&w, spec->width);
        put(&w, spec->width);
        put(&w, 0);
        put(&w, 0);
        put(&w, spec->delay);
        for (uint32_t p = 0; p < spec->width * spec->width; p++) {
            put(&w, spec->color);
        }
    }
    for (size_t i = 0; i < w.count; i++) {
        for (int b = 0; b < 4; b++) {
            out[i * 4 + (size_t) b] = (unsigned char) (w.words[i] >> (8 * b));
        }
    }
    return w.count * 4;
}

static bool load_request(const unsigned char *data, size_t length, const xcursor_request *request,
                         xcursor_frames *frames)
{
    FILE *file = fmemopen((void *) data, length, "r");
    bool loaded = xcursor_file_load(file, request, frames);
    fclose(file);
    return loaded;
}

static bool load_bytes(const unsigned char *data, size_t length, int size, xcursor_frames *frames)
{
    return load_request(data, length, &(xcursor_request) {size, 0, 0}, frames);
}

static const image_spec two_sizes[] = {
    {24, 2, 10, 0xff000001},
    {32, 3, 0, 0xff000002},
    {24, 2, 20, 0xff000003},
};

static void test_picks_nearest_size_with_all_frames(void)
{
    unsigned char data[MAX_WORDS * 4];
    size_t length = build_file(two_sizes, 3, data);
    xcursor_frames frames;
    CHECK(load_bytes(data, length, 27, &frames));
    CHECK_INT_EQ(2, (int) frames.count);
    CHECK_INT_EQ(2, (int) frames.frames[0].width);
    CHECK_INT_EQ(10, (int) frames.frames[0].delay);
    CHECK_INT_EQ(20, (int) frames.frames[1].delay);
    CHECK(frames.frames[1].pixels[3] == 0xff000003);
    xcursor_frames_clear(&frames);
}

static void test_larger_size_wins_when_closer(void)
{
    unsigned char data[MAX_WORDS * 4];
    size_t length = build_file(two_sizes, 3, data);
    xcursor_frames frames;
    CHECK(load_bytes(data, length, 30, &frames));
    CHECK_INT_EQ(1, (int) frames.count);
    CHECK_INT_EQ(3, (int) frames.frames[0].width);
    CHECK(frames.frames[0].pixels[8] == 0xff000002);
    xcursor_frames_clear(&frames);
}

static void test_rejects_bad_files(void)
{
    unsigned char data[MAX_WORDS * 4];
    size_t length = build_file(two_sizes, 3, data);
    xcursor_frames frames;
    CHECK(!load_bytes(data, length - 4, 24, &frames));
    data[0] = 'Y';
    CHECK(!load_bytes(data, length, 24, &frames));
    CHECK(frames.count == 0 && frames.frames == NULL);
}

static void test_loads_larger_when_width_differs(void)
{
    unsigned char data[MAX_WORDS * 4];
    size_t length = build_file(two_sizes, 3, data);
    xcursor_frames frames;
    CHECK(load_request(data, length, &(xcursor_request) {24, 2, 32}, &frames));
    CHECK(!frames.larger && frames.frames[0].width == 2);
    xcursor_frames_clear(&frames);
    CHECK(load_request(data, length, &(xcursor_request) {24, 5, 32}, &frames));
    CHECK(frames.larger && frames.count == 1 && frames.frames[0].width == 3);
    xcursor_frames_clear(&frames);
    CHECK(load_request(data, length, &(xcursor_request) {24, 5, 26}, &frames));
    CHECK(frames.larger && frames.count == 2 && frames.frames[0].width == 2);
    xcursor_frames_clear(&frames);
}

static void make_theme(const char *root, const char *theme, const char *inherits)
{
    char *cursors = g_build_filename(root, theme, "cursors", NULL);
    g_mkdir_with_parents(cursors, 0700);
    g_free(cursors);
    if (inherits != NULL) {
        char *path = g_build_filename(root, theme, "index.theme", NULL);
        char *text = g_strdup_printf("[Icon Theme]\nInherits = %s\n", inherits);
        g_file_set_contents(path, text, -1, NULL);
        g_free(text);
        g_free(path);
    }
}

static int remove_entry(const char *path, const struct stat *st, int flag, struct FTW *ftw)
{
    (void) st;
    (void) flag;
    (void) ftw;
    return remove(path);
}

static void test_theme_follows_inherits(void)
{
    char *root = g_mkdtemp(g_build_filename(g_get_tmp_dir(), "xcursor-test-XXXXXX", NULL));
    CHECK(root != NULL);
    make_theme(root, "first", "missing; second");
    make_theme(root, "second", "first");
    unsigned char data[MAX_WORDS * 4];
    size_t length = build_file(two_sizes, 3, data);
    char *path = g_build_filename(root, "second", "cursors", "default", NULL);
    g_file_set_contents(path, (const char *) data, (gssize) length, NULL);
    g_free(path);
    g_setenv("XCURSOR_PATH", root, TRUE);

    char **dirs = xcursor_theme_dirs("first");
    CHECK(g_strv_length(dirs) == 2);
    xcursor_frames frames;
    CHECK(xcursor_theme_load(dirs, "default", &(xcursor_request) {32, 0, 0}, &frames));
    CHECK_INT_EQ(3, (int) frames.frames[0].width);
    xcursor_frames_clear(&frames);
    CHECK(!xcursor_theme_load(dirs, "watch", &(xcursor_request) {32, 0, 0}, &frames));
    g_strfreev(dirs);
    dirs = xcursor_theme_dirs("absent");
    CHECK(dirs[0] == NULL);
    g_strfreev(dirs);

    nftw(root, remove_entry, 8, FTW_DEPTH | FTW_PHYS);
    g_free(root);
}

static void test_search_path_expands_home(void)
{
    g_setenv("XCURSOR_PATH", "~/themes:/usr/share/icons", TRUE);
    char **path = xcursor_search_path();
    char *home_themes = g_build_filename(g_get_home_dir(), "themes", NULL);
    CHECK(g_strv_length(path) == 2);
    CHECK(strcmp(home_themes, path[0]) == 0);
    CHECK(strcmp("/usr/share/icons", path[1]) == 0);
    g_free(home_themes);
    g_strfreev(path);
}

int main(void)
{
    const check_case cases[] = {
        {"picks_nearest_size_with_all_frames", test_picks_nearest_size_with_all_frames},
        {"larger_size_wins_when_closer", test_larger_size_wins_when_closer},
        {"rejects_bad_files", test_rejects_bad_files},
        {"loads_larger_when_width_differs", test_loads_larger_when_width_differs},
        {"theme_follows_inherits", test_theme_follows_inherits},
        {"search_path_expands_home", test_search_path_expands_home},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
