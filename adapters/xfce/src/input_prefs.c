#include "input_prefs.h"

#include <string.h>

#include "input_resolve.h"
#include "input_text.h"
#include "xfconf_source.h"

static const char default_prefix[] = "/Default/";
static const char vconsole_file[] = "/etc/vconsole.conf";
static const char xorg_file[] = "/etc/X11/xorg.conf";
static const char xorg_system_dir[] = "/usr/share/X11/xorg.conf.d";
static const char xorg_admin_dir[] = "/etc/X11/xorg.conf.d";
static const char xorg_suffix[] = ".conf";

typedef void (*keymap_reader)(system_keymap *keymap, const char *text);

static const char *lookup_environment(void *self, const char *key);
static void read_keymap_file(const char *path, system_keymap *keymap, keymap_reader read);
static void read_xorg_dir(const char *dir, const char *overriding_dir, system_keymap *keymap);
static gint compare_names(gconstpointer a, gconstpointer b);

system_keymap input_prefs_system_keymap(char **environment)
{
    system_keymap vconsole = { 0 };
    system_keymap xorg = { 0 };

    read_keymap_file(vconsole_file, &vconsole, system_keymap_read_vconsole);
    read_keymap_file(xorg_file, &xorg, system_keymap_read_xorg);
    read_xorg_dir(xorg_system_dir, xorg_admin_dir, &xorg);
    read_xorg_dir(xorg_admin_dir, NULL, &xorg);
    return system_keymap_choose(&vconsole, &xorg, environment);
}

keyboard_prefs input_prefs_keyboard(const channels *channels, const system_keymap *system)
{
    xfconf_source *layout =
        xfconf_source_create(channels_get(channels, CHANNEL_KEYBOARD_LAYOUT), default_prefix);
    xfconf_source *keyboards =
        xfconf_source_create(channels_get(channels, CHANNEL_KEYBOARDS), default_prefix);
    keyboard_sources sources = {
        .layout = xfconf_source_view(layout),
        .keyboards = xfconf_source_view(keyboards),
        .system = system,
    };
    keyboard_prefs prefs = input_keyboard_prefs_from(&sources);

    xfconf_source_destroy(layout);
    xfconf_source_destroy(keyboards);
    return prefs;
}

pointer_prefs input_prefs_pointer(const channels *channels, const char *device_name)
{
    char *prefix = input_device_property_prefix(device_name ? device_name : "");
    xfconf_source *device = xfconf_source_create(channels_get(channels, CHANNEL_POINTERS), prefix);
    setting_source view = xfconf_source_view(device);
    pointer_prefs prefs = input_pointer_prefs_from(&view);

    xfconf_source_destroy(device);
    g_free(prefix);
    return prefs;
}

cursor_prefs input_prefs_cursor(const channels *channels, char **environment)
{
    xfconf_source *xsettings = xfconf_source_create(channels_get(channels, CHANNEL_XSETTINGS), "/");
    cursor_sources sources = {
        .xsettings = xfconf_source_view(xsettings),
        .environment = { .self = environment, .lookup = lookup_environment },
    };
    cursor_prefs prefs = input_cursor_prefs_from(&sources);

    xfconf_source_destroy(xsettings);
    return prefs;
}

static const char *lookup_environment(void *self, const char *key)
{
    return g_environ_getenv(self, key);
}

static void read_keymap_file(const char *path, system_keymap *keymap, keymap_reader read)
{
    char *text = NULL;
    if (g_file_get_contents(path, &text, NULL, NULL)) {
        read(keymap, text);
    }
    g_free(text);
}

static void read_xorg_dir(const char *dir, const char *overriding_dir, system_keymap *keymap)
{
    GDir *handle = g_dir_open(dir, 0, NULL);
    if (handle == NULL) {
        return;
    }
    GPtrArray *names = g_ptr_array_new_with_free_func(g_free);
    for (const char *name; (name = g_dir_read_name(handle)) != NULL;) {
        if (g_str_has_suffix(name, xorg_suffix)) {
            g_ptr_array_add(names, g_strdup(name));
        }
    }
    g_dir_close(handle);
    g_ptr_array_sort(names, compare_names);
    for (guint i = 0; i < names->len; i++) {
        const char *name = g_ptr_array_index(names, i);
        char *path = g_build_filename(dir, name, NULL);
        char *override = overriding_dir ? g_build_filename(overriding_dir, name, NULL) : NULL;
        if (override == NULL || !g_file_test(override, G_FILE_TEST_EXISTS)) {
            read_keymap_file(path, keymap, system_keymap_read_xorg);
        }
        g_free(override);
        g_free(path);
    }
    g_ptr_array_free(names, TRUE);
}

static gint compare_names(gconstpointer a, gconstpointer b)
{
    return strcmp(*(char *const *) a, *(char *const *) b);
}
