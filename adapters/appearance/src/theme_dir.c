#include "theme_dir.h"

#include <stdbool.h>
#include <string.h>

#include <glib.h>

static const char default_theme[] = "Default";
static const char default_hdpi_theme[] = "Default-xhdpi";
static const char system_themes_dir[] = "/usr/share/themes";
static const char theme_subdir[] = "xfwm4";
static const char themerc_name[] = "themerc";

enum {
    HDPI_SCALE = 2,
    NORMAL_SCALE = 1,
};

static const char *scaled_name(const char *name, double max_output_scale);
static char *find_dir(const char *name);
static char *lookup_relative(const char *name);
static char *lookup_in(const char *themes_dir, const char *name);
static char *system_default_dir(void);

theme_location appearance_theme_locate(const char *name, double max_output_scale)
{
    const char *resolved = scaled_name(name, max_output_scale);
    char *hdpi_dir = strcmp(resolved, default_hdpi_theme) == 0 ? lookup_relative(resolved) : NULL;

    if (hdpi_dir != NULL && max_output_scale >= HDPI_SCALE) {
        return (theme_location) { hdpi_dir, HDPI_SCALE };
    }
    g_free(hdpi_dir);
    return (theme_location) { find_dir(resolved), NORMAL_SCALE };
}

void appearance_theme_location_clear(theme_location *location)
{
    g_clear_pointer(&location->dir, g_free);
}

static const char *scaled_name(const char *name, double max_output_scale)
{
    if (name == NULL || name[0] == '\0') {
        return max_output_scale >= HDPI_SCALE ? default_hdpi_theme : default_theme;
    }
    if (max_output_scale >= HDPI_SCALE && strcmp(name, default_theme) == 0) {
        return default_hdpi_theme;
    }
    return name;
}

static char *find_dir(const char *name)
{
    if (g_path_is_absolute(name)) {
        return g_file_test(name, G_FILE_TEST_IS_DIR) ? g_strdup(name) : system_default_dir();
    }
    char *dir = lookup_relative(name);
    if (dir == NULL) {
        dir = lookup_relative(default_theme);
    }
    return dir != NULL ? dir : system_default_dir();
}

static char *lookup_relative(const char *name)
{
    char *user_themes = g_build_filename(g_get_user_data_dir(), "themes", NULL);
    char *home_themes = g_build_filename(g_get_home_dir(), ".themes", NULL);
    char *dir = lookup_in(user_themes, name);

    if (dir == NULL) {
        dir = lookup_in(home_themes, name);
    }
    for (const char *const *data = g_get_system_data_dirs(); dir == NULL && *data; data++) {
        char *themes = g_build_filename(*data, "themes", NULL);
        dir = lookup_in(themes, name);
        g_free(themes);
    }
    if (dir == NULL) {
        dir = lookup_in(system_themes_dir, name);
    }
    g_free(user_themes);
    g_free(home_themes);
    return dir;
}

static char *lookup_in(const char *themes_dir, const char *name)
{
    char *dir = g_build_filename(themes_dir, name, theme_subdir, NULL);
    char *rc = g_build_filename(dir, themerc_name, NULL);
    bool found = g_file_test(rc, G_FILE_TEST_IS_REGULAR);

    g_free(rc);
    if (!found) {
        g_free(dir);
        return NULL;
    }
    return dir;
}

static char *system_default_dir(void)
{
    return g_build_filename(system_themes_dir, default_theme, theme_subdir, NULL);
}
