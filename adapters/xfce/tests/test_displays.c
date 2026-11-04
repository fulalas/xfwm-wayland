#include <glib.h>
#include <string.h>

#include "check.h"
#include "display_choice.h"
#include "display_hash.h"
#include "display_plan.h"
#include "display_read.h"
#include "display_table.h"
#include "display_transform.h"
#include "display_write.h"

static const char abc_hash[] = "a9993e364706816aba3e25717850c26c9cd0d89d";
static const char headless_hash[] = "cdd93bd1d3a657be9862fdd1a83e6fe458f86ac2";
static const uint8_t abc_edid[] = { 'a', 'b', 'c' };

typedef struct entry {
    const char *key;
    const char *value;
} entry;

static GHashTable *table_of(const entry *entries)
{
    GHashTable *table = display_table_new();

    for (size_t i = 0; entries[i].key; i++) {
        g_hash_table_insert(table, g_strdup(entries[i].key), g_strdup(entries[i].value));
    }
    return table;
}

static display_saved read_entry(GHashTable *table, const char *connector, const char *hash)
{
    display_lookup lookup = {
        .table = table, .profile = "Default", .connector = connector, .hash = hash,
    };

    return display_profile_read(&lookup);
}

static char *choose(const entry *entries, const char *first, const char *second)
{
    GHashTable *table = table_of(entries);
    char *hashes[] = { (char *) first, (char *) second, NULL };
    char *profile = display_profile_choose(table, hashes);

    g_hash_table_destroy(table);
    return profile;
}

static const entry two_profiles[] = {
    { "/Default/eDP-1", "Panel" }, { "/Default/eDP-1/EDID", "aaa" },
    { "/Default/eDP-1/Active", "true" },
    { "/Home", "Home" }, { "/Home/eDP-1", "Panel" }, { "/Home/eDP-1/EDID", "aaa" },
    { "/Home/DP-1", "Big" }, { "/Home/DP-1/EDID", "bbb" },
    { "/Work", "Work" }, { "/Work/DP-2", "Other" }, { "/Work/DP-2/EDID", "ccc" },
    { NULL, NULL },
};

static void test_hash_uses_edid_bytes(void)
{
    display_identity identity = { .connector = "DP-1", .edid = abc_edid, .edid_size = 3 };
    char *hash = display_hash(&identity);

    CHECK(strcmp(hash, abc_hash) == 0);
    g_free(hash);
}

static void test_hash_without_edid_matches_settings_dialog(void)
{
    display_identity identity = { .connector = "HEADLESS-1", .make = "headless",
                                  .model = "headless" };
    char *hash = display_hash(&identity);

    CHECK(strcmp(hash, headless_hash) == 0);
    g_free(hash);
}

static void test_transform_from_reflection(void)
{
    display_rotation y_quarter = { 90, "Y" };
    display_rotation xy_half = { 180, "XY" };
    display_rotation odd = { 45, "Z" };

    CHECK_INT_EQ(DISPLAY_TRANSFORM_FLIPPED_270, display_transform_from(&y_quarter));
    CHECK_INT_EQ(DISPLAY_TRANSFORM_NORMAL, display_transform_from(&xy_half));
    CHECK_INT_EQ(DISPLAY_TRANSFORM_NORMAL, display_transform_from(&odd));
}

static void test_transform_round_trip(void)
{
    for (int t = DISPLAY_TRANSFORM_NORMAL; t <= DISPLAY_TRANSFORM_FLIPPED_270; t++) {
        display_rotation rotation = display_rotation_from((display_transform) t);

        CHECK_INT_EQ(t, display_transform_from(&rotation));
    }
}

static void test_choose_single_matching_user_profile(void)
{
    char *profile = choose(two_profiles, "bbb", "aaa");

    CHECK(strcmp(profile, "Home") == 0);
    g_free(profile);
}

static void test_choose_default_without_matching_profile(void)
{
    char *profile = choose(two_profiles, "aaa", NULL);

    CHECK(strcmp(profile, "Default") == 0);
    g_free(profile);
}

static void test_choose_default_when_auto_enable_is_off(void)
{
    entry entries[G_N_ELEMENTS(two_profiles) + 1];

    memcpy(entries, two_profiles, sizeof(two_profiles));
    entries[G_N_ELEMENTS(two_profiles) - 1] = (entry) { "/AutoEnableProfiles", "0" };
    entries[G_N_ELEMENTS(two_profiles)] = (entry) { NULL, NULL };
    char *profile = choose(entries, "bbb", "aaa");

    CHECK(strcmp(profile, "Default") == 0);
    g_free(profile);
}

static void test_choose_default_when_two_profiles_match(void)
{
    static const entry twins[] = {
        { "/A", "A" }, { "/A/DP-1", "x" }, { "/A/DP-1/EDID", "aaa" },
        { "/B", "B" }, { "/B/DP-1", "x" }, { "/B/DP-1/EDID", "aaa" },
        { NULL, NULL },
    };
    char *profile = choose(twins, "aaa", NULL);

    CHECK(strcmp(profile, "Default") == 0);
    g_free(profile);
}

static const entry saved_outputs[] = {
    { "/Default/DP-1", "Big" }, { "/Default/DP-1/EDID", "bbb" },
    { "/Default/DP-1/Active", "true" }, { "/Default/DP-1/Resolution", "2560x1440" },
    { "/Default/DP-1/RefreshRate", "59.951000000000001" },
    { "/Default/DP-1/Rotation", "90" }, { "/Default/DP-1/Reflection", "X" },
    { "/Default/DP-1/Scale", "1.5" }, { "/Default/DP-1/Position/X", "1920" },
    { "/Default/DP-1/Position/Y", "-10" }, { "/Default/DP-1/Primary", "true" },
    { "/Default/eDP-1", "Panel" }, { "/Default/eDP-1/EDID", "aaa" },
    { "/Default/eDP-1/Active", "false" },
    { "/Default/HDMI-A-1", "Twin" }, { "/Default/HDMI-A-1/EDID", "ccc" },
    { "/Default/HDMI-A-1/DuplicateEDID", "true" }, { "/Default/HDMI-A-1/Active", "true" },
    { NULL, NULL },
};

static void test_read_matches_edid_under_another_connector(void)
{
    GHashTable *table = table_of(saved_outputs);
    display_saved saved = read_entry(table, "DP-3", "bbb");

    CHECK(saved.is_known && saved.is_primary && saved.setting.enabled);
    CHECK_INT_EQ(2560, saved.setting.mode.width);
    CHECK_INT_EQ(1440, saved.setting.mode.height);
    CHECK_INT_EQ(59951, saved.setting.mode.refresh_mhz);
    CHECK_INT_EQ(DISPLAY_TRANSFORM_FLIPPED_90, saved.setting.transform);
    CHECK_DOUBLE_NEAR(1.5, saved.setting.scale, 1e-9);
    CHECK_INT_EQ(1920, saved.setting.x);
    CHECK_INT_EQ(-10, saved.setting.y);
    g_hash_table_destroy(table);
}

static void test_read_inactive_output(void)
{
    GHashTable *table = table_of(saved_outputs);
    display_saved saved = read_entry(table, "eDP-1", "aaa");

    CHECK(saved.is_known && !saved.setting.enabled && !saved.is_primary);
    g_hash_table_destroy(table);
}

static void test_read_duplicate_edid_matches_by_name(void)
{
    GHashTable *table = table_of(saved_outputs);
    display_saved by_name = read_entry(table, "HDMI-A-1", "zzz");
    display_saved by_edid = read_entry(table, "HDMI-A-2", "ccc");

    CHECK(by_name.is_known && by_name.setting.enabled);
    CHECK_DOUBLE_NEAR(1.0, by_name.setting.scale, 1e-9);
    CHECK_INT_EQ(0, by_name.setting.mode.width);
    CHECK_INT_EQ(DISPLAY_TRANSFORM_NORMAL, by_name.setting.transform);
    CHECK(by_edid.is_known);
    g_hash_table_destroy(table);
}

static void test_read_unknown_output(void)
{
    GHashTable *table = table_of(saved_outputs);

    CHECK(!read_entry(table, "DP-9", "fff").is_known);
    g_hash_table_destroy(table);
}

static void test_plan_keeps_one_primary(void)
{
    static const entry entries[] = {
        { "/Default/A", "a" }, { "/Default/A/EDID", abc_hash },
        { "/Default/A/Active", "true" }, { "/Default/A/Primary", "true" },
        { "/Default/B", "b" }, { "/Default/B/EDID", headless_hash },
        { "/Default/B/Active", "true" }, { "/Default/B/Primary", "true" },
        { NULL, NULL },
    };
    display_state displays[] = {
        { .identity = { .connector = "A", .edid = abc_edid, .edid_size = 3 } },
        { .identity = { .connector = "B", .make = "headless", .model = "headless" } },
    };
    display_set connected = { displays, 2 };
    GHashTable *table = table_of(entries);
    display_plan plan = display_profile_plan(table, &connected);

    CHECK(plan.displays[0].is_known && plan.displays[1].is_known);
    CHECK(plan.displays[0].is_primary && !plan.displays[1].is_primary);
    g_hash_table_destroy(table);
}

static const char *written(const GPtrArray *writes, const char *key)
{
    for (guint i = 0; i < writes->len; i++) {
        const profile_write *write = g_ptr_array_index(writes, i);

        if (strcmp(write->key, key) == 0) {
            return write->text;
        }
    }
    return NULL;
}

static const display_state two_displays[] = {
    {
        .identity = { .connector = "DP-1", .description = "Big", .edid = abc_edid,
                      .edid_size = 3 },
        .setting = { .enabled = true, .mode = { 2560, 1440, 59951 }, .x = 1920, .y = 0,
                     .transform = DISPLAY_TRANSFORM_FLIPPED_270, .scale = 1.5 },
        .is_primary = true,
    },
    {
        .identity = { .connector = "eDP-1", .make = "headless", .model = "headless" },
        .setting = { .enabled = false, .scale = 1.0 },
    },
};

static void test_writes_follow_settings_dialog_layout(void)
{
    display_set current = { two_displays, 2 };
    GPtrArray *writes = display_profile_writes(&current);

    CHECK(strcmp(written(writes, "/Default/DP-1"), "Big") == 0);
    CHECK(strcmp(written(writes, "/Default/DP-1/EDID"), abc_hash) == 0);
    CHECK(strcmp(written(writes, "/Default/DP-1/Resolution"), "2560x1440") == 0);
    CHECK(strcmp(written(writes, "/Default/DP-1/RefreshRate"), "59.951000000000001") == 0);
    CHECK(strcmp(written(writes, "/Default/DP-1/Rotation"), "270") == 0);
    CHECK(strcmp(written(writes, "/Default/DP-1/Reflection"), "X") == 0);
    CHECK(strcmp(written(writes, "/Default/DP-1/Scale"), "1.5") == 0);
    CHECK(strcmp(written(writes, "/Default/DP-1/Position/X"), "1920") == 0);
    CHECK(strcmp(written(writes, "/Default/DP-1/Primary"), "true") == 0);
    CHECK(strcmp(written(writes, "/Default/eDP-1"), "eDP-1") == 0);
    CHECK(strcmp(written(writes, "/Default/eDP-1/Active"), "false") == 0);
    CHECK(written(writes, "/Default/eDP-1/Resolution") == NULL);
    CHECK(written(writes, "/Default/eDP-1/Primary") == NULL);
    g_ptr_array_unref(writes);
}

static void test_writes_mark_duplicate_edids(void)
{
    display_state twins[] = { two_displays[0], two_displays[0] };
    display_set current = { twins, 2 };
    GPtrArray *writes;

    twins[1].identity.connector = "DP-2";
    writes = display_profile_writes(&current);
    CHECK(strcmp(written(writes, "/Default/DP-1/DuplicateEDID"), "true") == 0);
    CHECK(strcmp(written(writes, "/Default/DP-2/DuplicateEDID"), "true") == 0);
    g_ptr_array_unref(writes);
}

static void test_written_profile_reads_back(void)
{
    display_set current = { two_displays, 2 };
    GPtrArray *writes = display_profile_writes(&current);
    GHashTable *table = display_table_new();
    display_saved saved;

    for (guint i = 0; i < writes->len; i++) {
        const profile_write *write = g_ptr_array_index(writes, i);

        g_hash_table_insert(table, g_strdup(write->key), g_strdup(write->text));
    }
    saved = read_entry(table, "DP-1", abc_hash);
    CHECK(display_profile_is_saved(table, writes));
    CHECK_INT_EQ(59951, saved.setting.mode.refresh_mhz);
    CHECK_INT_EQ(DISPLAY_TRANSFORM_FLIPPED_270, saved.setting.transform);
    g_hash_table_insert(table, g_strdup("/Default/DP-1/Scale"), g_strdup("2"));
    CHECK(!display_profile_is_saved(table, writes));
    g_hash_table_destroy(table);
    g_ptr_array_unref(writes);
}

int main(void)
{
    const check_case cases[] = {
        { "hash_uses_edid_bytes", test_hash_uses_edid_bytes },
        { "hash_without_edid_matches_settings_dialog",
          test_hash_without_edid_matches_settings_dialog },
        { "transform_from_reflection", test_transform_from_reflection },
        { "transform_round_trip", test_transform_round_trip },
        { "choose_single_matching_user_profile", test_choose_single_matching_user_profile },
        { "choose_default_without_matching_profile",
          test_choose_default_without_matching_profile },
        { "choose_default_when_auto_enable_is_off", test_choose_default_when_auto_enable_is_off },
        { "choose_default_when_two_profiles_match", test_choose_default_when_two_profiles_match },
        { "read_matches_edid_under_another_connector",
          test_read_matches_edid_under_another_connector },
        { "read_inactive_output", test_read_inactive_output },
        { "read_duplicate_edid_matches_by_name", test_read_duplicate_edid_matches_by_name },
        { "read_unknown_output", test_read_unknown_output },
        { "plan_keeps_one_primary", test_plan_keeps_one_primary },
        { "writes_follow_settings_dialog_layout", test_writes_follow_settings_dialog_layout },
        { "writes_mark_duplicate_edids", test_writes_mark_duplicate_edids },
        { "written_profile_reads_back", test_written_profile_reads_back },
    };

    return check_run(cases, G_N_ELEMENTS(cases));
}
