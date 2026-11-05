#include "display_read.h"

#include <stdio.h>
#include <string.h>

#include "display_choice.h"
#include "display_table.h"
#include "display_transform.h"

enum {
    MILLIHERTZ_PER_HERTZ = 1000,
    MAX_REFRESH_HZ = 1000,
};

static const char edid_suffix[] = "/EDID";

static char *entry_name(const display_lookup *lookup);
static char *entry_with_edid(const display_lookup *lookup);
static char *output_of_edid_key(const char *key, const char *profile);
static display_setting read_setting(GHashTable *table, const char *profile, const char *name);
static display_mode read_mode(GHashTable *table, display_key *key);
static display_transform read_transform(GHashTable *table, display_key *key);
static int millihertz(double hertz);

display_saved display_profile_read(const display_lookup *lookup)
{
    display_saved saved = { 0 };
    char *name = entry_name(lookup);
    display_key active = { .profile = lookup->profile, .connector = name, .leaf = "Active" };
    display_key primary = { .profile = lookup->profile, .connector = name, .leaf = "Primary" };

    if (!name || !display_table_text(lookup->table, &active)) {
        g_free(name);
        return saved;
    }
    saved.is_known = true;
    saved.is_primary = display_table_bool(lookup->table, &primary);
    saved.setting = read_setting(lookup->table, lookup->profile, name);
    g_free(name);
    return saved;
}

static char *entry_name(const display_lookup *lookup)
{
    display_key duplicate = {
        .profile = lookup->profile, .connector = lookup->connector, .leaf = "DuplicateEDID",
    };

    if (display_table_bool(lookup->table, &duplicate)) {
        return g_strdup(lookup->connector);
    }
    return entry_with_edid(lookup);
}

static char *entry_with_edid(const display_lookup *lookup)
{
    GHashTableIter iter;
    void *key;
    void *value;

    g_hash_table_iter_init(&iter, lookup->table);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        char *name = strcmp(value, lookup->hash) == 0
                         ? output_of_edid_key(key, lookup->profile)
                         : NULL;
        if (name) {
            return name;
        }
    }
    return NULL;
}

static char *output_of_edid_key(const char *key, const char *profile)
{
    char *output_key;
    char *name;

    if (!g_str_has_suffix(key, edid_suffix)) {
        return NULL;
    }
    output_key = g_strndup(key, strlen(key) - strlen(edid_suffix));
    name = display_profile_output_name(output_key, profile);
    g_free(output_key);
    return name;
}

static display_setting read_setting(GHashTable *table, const char *profile, const char *name)
{
    display_key key = { .profile = profile, .connector = name, .leaf = "Active" };
    display_setting setting = { .enabled = display_table_bool(table, &key), .scale = 1.0 };

    if (!setting.enabled) {
        return setting;
    }
    setting.mode = read_mode(table, &key);
    setting.transform = read_transform(table, &key);
    key.leaf = "Scale";
    setting.scale = display_table_double(table, &key, 1.0);
    setting.scale = setting.scale > 0.0 ? setting.scale : 1.0;
    key.leaf = "Position/X";
    setting.x = display_table_int(table, &key, 0);
    key.leaf = "Position/Y";
    setting.y = display_table_int(table, &key, 0);
    return setting;
}

static display_mode read_mode(GHashTable *table, display_key *key)
{
    display_mode mode = { 0 };
    const char *resolution;
    char trailing;

    key->leaf = "Resolution";
    resolution = display_table_text(table, key);
    if (!resolution ||
        sscanf(resolution, "%dx%d%c", &mode.width, &mode.height, &trailing) != 2) {
        return (display_mode) { 0 };
    }
    key->leaf = "RefreshRate";
    mode.refresh_mhz = millihertz(display_table_double(table, key, 0.0));
    return mode;
}

static display_transform read_transform(GHashTable *table, display_key *key)
{
    display_rotation rotation;

    key->leaf = "Rotation";
    rotation.degrees = display_table_int(table, key, 0);
    key->leaf = "Reflection";
    rotation.reflection = display_table_text(table, key);
    return display_transform_from(&rotation);
}

static int millihertz(double hertz)
{
    if (hertz <= 0.0 || hertz > MAX_REFRESH_HZ) {
        return 0;
    }
    return (int) (hertz * MILLIHERTZ_PER_HERTZ + 0.5);
}
