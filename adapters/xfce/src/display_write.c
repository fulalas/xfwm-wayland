#include "display_write.h"

#include <string.h>

#include "display_choice.h"
#include "display_hash.h"
#include "display_table.h"
#include "display_transform.h"

enum {
    MILLIHERTZ_PER_HERTZ = 1000,
};

typedef struct profile_value {
    profile_value_kind kind;
    char *text;
} profile_value;

typedef struct output_writer {
    GPtrArray *writes;
    const char *connector;
} output_writer;

static void write_display(GPtrArray *writes, const display_set *current, size_t index);
static void write_setting(const output_writer *writer, const display_setting *setting);
static bool has_duplicate(const display_set *current, size_t index, const char *hash);
static void add(const output_writer *writer, const char *leaf, profile_value value);
static profile_value text_value(const char *text);
static profile_value bool_value(bool value);
static profile_value int_value(int value);
static profile_value double_value(double value);
static void free_write(void *data);

GPtrArray *display_profile_writes(const display_set *current)
{
    GPtrArray *writes = g_ptr_array_new_with_free_func(free_write);

    for (size_t i = 0; i < current->count; i++) {
        write_display(writes, current, i);
    }
    return writes;
}

bool display_profile_is_saved(GHashTable *table, const GPtrArray *writes)
{
    for (guint i = 0; i < writes->len; i++) {
        const profile_write *write = g_ptr_array_index(writes, i);
        const char *saved = g_hash_table_lookup(table, write->key);

        if (!saved || strcmp(saved, write->text) != 0) {
            return false;
        }
    }
    return true;
}

static void write_display(GPtrArray *writes, const display_set *current, size_t index)
{
    const display_state *display = &current->displays[index];
    const display_identity *identity = &display->identity;
    output_writer writer = { .writes = writes, .connector = identity->connector };
    char *hash = display_hash(identity);
    const char *name = identity->description ? identity->description : identity->connector;

    add(&writer, NULL, text_value(name));
    add(&writer, "EDID", text_value(hash));
    add(&writer, "DuplicateEDID", bool_value(has_duplicate(current, index, hash)));
    add(&writer, "Active", bool_value(display->setting.enabled));
    if (display->setting.enabled) {
        write_setting(&writer, &display->setting);
    }
    if (display->is_primary) {
        add(&writer, "Primary", bool_value(true));
    }
    g_free(hash);
}

static void write_setting(const output_writer *writer, const display_setting *setting)
{
    display_rotation rotation = display_rotation_from(setting->transform);
    double refresh_hz = (double) setting->mode.refresh_mhz / MILLIHERTZ_PER_HERTZ;

    char *resolution = g_strdup_printf("%dx%d", setting->mode.width, setting->mode.height);

    add(writer, "Resolution", text_value(resolution));
    add(writer, "RefreshRate", double_value(refresh_hz));
    add(writer, "Rotation", int_value(rotation.degrees));
    add(writer, "Reflection", text_value(rotation.reflection));
    add(writer, "Scale", double_value(setting->scale));
    add(writer, "Position/X", int_value(setting->x));
    add(writer, "Position/Y", int_value(setting->y));
    g_free(resolution);
}

static bool has_duplicate(const display_set *current, size_t index, const char *hash)
{
    bool found = false;

    for (size_t i = 0; i < current->count && !found; i++) {
        char *other = i != index ? display_hash(&current->displays[i].identity) : NULL;

        found = other && strcmp(other, hash) == 0;
        g_free(other);
    }
    return found;
}

static void add(const output_writer *writer, const char *leaf, profile_value value)
{
    display_key key = {
        .profile = display_default_profile, .connector = writer->connector, .leaf = leaf,
    };
    profile_write *write = g_new0(profile_write, 1);

    write->key = display_key_text(&key);
    write->kind = value.kind;
    write->text = value.text;
    g_ptr_array_add(writer->writes, write);
}

static profile_value text_value(const char *text)
{
    return (profile_value) { .kind = PROFILE_VALUE_TEXT, .text = g_strdup(text) };
}

static profile_value bool_value(bool value)
{
    const char *text = value ? "true" : "false";

    return (profile_value) { .kind = PROFILE_VALUE_BOOL, .text = g_strdup(text) };
}

static profile_value int_value(int value)
{
    return (profile_value) { .kind = PROFILE_VALUE_INT, .text = g_strdup_printf("%d", value) };
}

static profile_value double_value(double value)
{
    return (profile_value) { .kind = PROFILE_VALUE_DOUBLE, .text = display_double_text(value) };
}

static void free_write(void *data)
{
    profile_write *write = data;

    g_free(write->key);
    g_free(write->text);
    g_free(write);
}
