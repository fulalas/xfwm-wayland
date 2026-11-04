#include "display_choice.h"

#include <stdbool.h>
#include <string.h>

#include "display_table.h"

enum auto_enable_mode {
    AUTO_ENABLE_NEVER,
    AUTO_ENABLE_ON_CONNECT,
    AUTO_ENABLE_ON_DISCONNECT,
    AUTO_ENABLE_ALWAYS,
};

typedef struct output_census {
    guint outputs;
    guint with_edid;
    guint matching;
} output_census;

typedef struct profile_match {
    GHashTable *table;
    char *const *hashes;
    char *found;
    int count;
} profile_match;

const char display_default_profile[] = "Default";

static const char auto_enable_key[] = "AutoEnableProfiles";

static bool allows_auto_enable(GHashTable *table);
static void match_profile(void *key, void *value, void *data);
static char *top_level_name(const char *key);
static bool is_matching_user_profile(const profile_match *match, const char *profile);
static output_census take_census(const profile_match *match, const char *profile);
static void count_output(output_census *census, const profile_match *match,
                         const display_key *output);

char *display_profile_choose(GHashTable *table, char *const *hashes)
{
    profile_match match = { .table = table, .hashes = hashes };

    if (allows_auto_enable(table)) {
        g_hash_table_foreach(table, match_profile, &match);
    }
    if (match.count != 1) {
        g_free(match.found);
        return g_strdup(display_default_profile);
    }
    return match.found;
}

char *display_profile_output_name(const char *key, const char *profile)
{
    size_t length = strlen(profile);
    const char *name;

    if (key[0] != '/' || strncmp(key + 1, profile, length) != 0 || key[1 + length] != '/') {
        return NULL;
    }
    name = key + 1 + length + 1;
    return *name != '\0' && !strchr(name, '/') ? g_strdup(name) : NULL;
}

static bool allows_auto_enable(GHashTable *table)
{
    display_key key = { .profile = auto_enable_key };
    int mode = display_table_int(table, &key, AUTO_ENABLE_ALWAYS);

    return mode == AUTO_ENABLE_ON_CONNECT || mode == AUTO_ENABLE_ALWAYS;
}

static void match_profile(void *key, void *value, void *data)
{
    profile_match *match = data;
    char *profile = top_level_name(key);

    (void) value;
    if (profile && is_matching_user_profile(match, profile)) {
        match->count++;
        g_free(match->found);
        match->found = g_strdup(profile);
    }
    g_free(profile);
}

static char *top_level_name(const char *key)
{
    return key[0] == '/' && key[1] != '\0' && !strchr(key + 1, '/') ? g_strdup(key + 1) : NULL;
}

static bool is_matching_user_profile(const profile_match *match, const char *profile)
{
    output_census census = take_census(match, profile);

    return census.with_edid > 0 && census.outputs == census.matching &&
           census.outputs == g_strv_length((char **) match->hashes);
}

static output_census take_census(const profile_match *match, const char *profile)
{
    output_census census = { 0 };
    GHashTableIter iter;
    void *key;

    g_hash_table_iter_init(&iter, match->table);
    while (g_hash_table_iter_next(&iter, &key, NULL)) {
        char *name = display_profile_output_name(key, profile);
        display_key output = { .profile = profile, .connector = name, .leaf = "EDID" };

        if (name) {
            count_output(&census, match, &output);
        }
        g_free(name);
    }
    return census;
}

static void count_output(output_census *census, const profile_match *match,
                         const display_key *output)
{
    const char *edid = display_table_text(match->table, output);

    census->outputs++;
    if (!edid) {
        return;
    }
    census->with_edid++;
    if (g_strv_contains((const char *const *) match->hashes, edid)) {
        census->matching++;
    }
}
