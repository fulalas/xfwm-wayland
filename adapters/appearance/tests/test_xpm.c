#include <stdbool.h>
#include <string.h>

#include <glib.h>

#include "check.h"
#include "fixture.h"
#include "xpm.h"

static const xpm_symbol symbol_items[] = {
    { "active_color_1", "#3584e4" },
    { "active_mid_2", "#102030405060" },
};

static const xpm_symbols symbols = { symbol_items, 2 };

static bool entry_is(const char *entry, const char *expected)
{
    char *color = appearance_xpm_entry_color(entry, &symbols);
    bool equal = color != NULL && expected != NULL && strcmp(color, expected) == 0;
    bool both_null = color == NULL && expected == NULL;

    g_free(color);
    return equal || both_null;
}

static void test_entry_plain_color(void)
{
    CHECK(entry_is(" c #ff0000", "#ff0000"));
    CHECK(entry_is(" c None", "None"));
    CHECK(entry_is(" c light blue", "light blue"));
}

static void test_entry_symbol_overrides_color(void)
{
    CHECK(entry_is(" c #C0C0FF s active_mid_2", "#102030405060"));
    CHECK(entry_is(" s ACTIVE_COLOR_1 c #000000", "#3584e4"));
}

static void test_entry_unknown_symbol_keeps_color(void)
{
    CHECK(entry_is(" c #C0C0FF s unknown_symbol", "#C0C0FF"));
    CHECK(entry_is(" s unknown_symbol", NULL));
}

static void test_entry_prefers_color_key(void)
{
    CHECK(entry_is(" m white c red", "red"));
    CHECK(entry_is(" c red m white", "red"));
}

static void test_entry_incomplete_is_rejected(void)
{
    CHECK(entry_is(" c", NULL));
    CHECK(entry_is(" #ff0000", NULL));
}

static const char tiny_xpm[] = "/* XPM */\n"
                               "static char * tiny_xpm[] = {\n"
                               "/* width height colors chars */\n"
                               "\"3 2 3 1\",\n"
                               "\". c #C0C0FF s active_color_1\",\n"
                               "\"# c None\",\n"
                               "\"r c red\",\n"
                               "\".#r\",\n"
                               "\"r.?\"};\n";

static void check_pixel(const GdkPixbuf *pixbuf, int index, const guchar expected[4])
{
    int width = gdk_pixbuf_get_width(pixbuf);
    const guchar *pixels = gdk_pixbuf_read_pixels(pixbuf);
    const guchar *pixel = pixels + (index / width) * gdk_pixbuf_get_rowstride(pixbuf)
                          + (index % width) * 4;

    CHECK(memcmp(pixel, expected, 4) == 0);
}

static void test_load_substitutes_symbols(void)
{
    static const guchar symbol_blue[4] = { 0x35, 0x84, 0xE4, 0xFF };
    static const guchar transparent[4] = { 0, 0, 0, 0 };
    static const guchar red[4] = { 0xFF, 0, 0, 0xFF };
    char *dir = fixture_dir_create();
    char *path = fixture_write(dir, "tiny.xpm", tiny_xpm);
    GdkPixbuf *pixbuf = appearance_xpm_load(path, &symbols);

    CHECK(pixbuf != NULL);
    if (pixbuf != NULL) {
        CHECK_INT_EQ(3, gdk_pixbuf_get_width(pixbuf));
        CHECK_INT_EQ(2, gdk_pixbuf_get_height(pixbuf));
        check_pixel(pixbuf, 0, symbol_blue);
        check_pixel(pixbuf, 1, transparent);
        check_pixel(pixbuf, 2, red);
        check_pixel(pixbuf, 3, red);
        check_pixel(pixbuf, 5, symbol_blue);
        g_object_unref(pixbuf);
    }
    g_free(path);
    fixture_dir_remove(dir);
}

static void test_load_rejects_bad_header(void)
{
    char *dir = fixture_dir_create();
    char *path = fixture_write(dir, "bad.xpm", "/* XPM */ static char *x[] = { \"0 2 1 1\" };");

    CHECK(appearance_xpm_load(path, &symbols) == NULL);
    CHECK(appearance_xpm_load("/tmp/xfwm-appearance-missing.xpm", &symbols) == NULL);
    g_free(path);
    fixture_dir_remove(dir);
}

static void test_load_rejects_huge_image(void)
{
    char *dir = fixture_dir_create();
    char *path = fixture_write(dir, "huge.xpm", "/* XPM */ static char *x[] = { \"4097 1 1 1\" };");

    CHECK(appearance_xpm_load(path, &symbols) == NULL);
    g_free(path);
    fixture_dir_remove(dir);
}

int main(void)
{
    static const check_case cases[] = {
        { "entry_plain_color", test_entry_plain_color },
        { "entry_symbol_overrides_color", test_entry_symbol_overrides_color },
        { "entry_unknown_symbol_keeps_color", test_entry_unknown_symbol_keeps_color },
        { "entry_prefers_color_key", test_entry_prefers_color_key },
        { "entry_incomplete_is_rejected", test_entry_incomplete_is_rejected },
        { "load_substitutes_symbols", test_load_substitutes_symbols },
        { "load_rejects_bad_header", test_load_rejects_bad_header },
        { "load_rejects_huge_image", test_load_rejects_huge_image },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
