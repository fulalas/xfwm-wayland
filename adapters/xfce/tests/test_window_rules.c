#include <string.h>

#include "check.h"
#include "window_rules.h"

static GHashTable *properties_new(void)
{
    return g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify) g_free);
}

static void add_text(GHashTable *properties, const char *key, const char *text)
{
    GValue *value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_STRING);
    g_value_set_string(value, text);
    g_hash_table_insert(properties, (char *) key, value);
}

static void add_int(GHashTable *properties, const char *key, int number)
{
    GValue *value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_INT);
    g_value_set_int(value, number);
    g_hash_table_insert(properties, (char *) key, value);
}

static void add_bool(GHashTable *properties, const char *key, bool on)
{
    GValue *value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_BOOLEAN);
    g_value_set_boolean(value, on);
    g_hash_table_insert(properties, (char *) key, value);
}

static void test_reads_every_field(void)
{
    GHashTable *properties = properties_new();
    add_text(properties, "/window-rules/0/title", "Notes*");
    add_text(properties, "/window-rules/0/app_id", "org.xfce.mousepad");
    add_text(properties, "/window-rules/0/match", "glob");
    add_text(properties, "/window-rules/0/position", "bottom-right");
    add_int(properties, "/window-rules/0/margin", 12);
    add_text(properties, "/window-rules/0/output", "DP-1");
    add_int(properties, "/window-rules/0/workspace", 2);
    add_bool(properties, "/window-rules/0/above", true);
    add_bool(properties, "/window-rules/0/skip_taskbar", false);
    add_bool(properties, "/window-rules/0/no_focus", true);
    xfce_window_rules rules = window_rules_from(properties);
    CHECK_INT_EQ(1, rules.count);
    const wm_window_rule *rule = &rules.items[0];
    CHECK(strcmp(rule->title, "Notes*") == 0);
    CHECK(strcmp(rule->app_id, "org.xfce.mousepad") == 0);
    CHECK_INT_EQ(WM_MATCH_GLOB, rule->match);
    CHECK_INT_EQ(WM_ANCHOR_BOTTOM_RIGHT, rule->anchor);
    CHECK_INT_EQ(12, rule->margin);
    CHECK(strcmp(rule->output, "DP-1") == 0);
    CHECK(rule->has_workspace);
    CHECK_INT_EQ(1, rule->workspace);
    CHECK(rule->above.is_set && rule->above.on);
    CHECK(rule->skip_taskbar.is_set && !rule->skip_taskbar.on);
    CHECK(rule->no_focus.is_set && rule->no_focus.on);
    xfce_window_rules_free(&rules);
    g_hash_table_destroy(properties);
}

static void test_rules_are_ordered_by_number(void)
{
    GHashTable *properties = properties_new();
    add_text(properties, "/window-rules/10/app_id", "ten");
    add_text(properties, "/window-rules/2/app_id", "two");
    add_text(properties, "/window-rules/extra/app_id", "extra");
    xfce_window_rules rules = window_rules_from(properties);
    CHECK_INT_EQ(3, rules.count);
    CHECK(strcmp(rules.items[0].app_id, "two") == 0);
    CHECK(strcmp(rules.items[1].app_id, "ten") == 0);
    CHECK(strcmp(rules.items[2].app_id, "extra") == 0);
    xfce_window_rules_free(&rules);
    g_hash_table_destroy(properties);
}

static void test_defaults_and_bad_values(void)
{
    GHashTable *properties = properties_new();
    add_text(properties, "/window-rules/1/app_id", "app");
    add_text(properties, "/window-rules/1/position", "Top_Left");
    add_int(properties, "/window-rules/1/margin", -5);
    add_int(properties, "/window-rules/1/workspace", 0);
    add_text(properties, "/window-rules/1/title", "");
    add_text(properties, "/window-rules/2/position", "nowhere");
    add_text(properties, "/window-rules/2/sub/title", "ignored");
    add_text(properties, "/other/1/title", "ignored");
    xfce_window_rules rules = window_rules_from(properties);
    CHECK_INT_EQ(2, rules.count);
    CHECK_INT_EQ(WM_ANCHOR_TOP_LEFT, rules.items[0].anchor);
    CHECK_INT_EQ(0, rules.items[0].margin);
    CHECK(!rules.items[0].has_workspace);
    CHECK(rules.items[0].title == NULL);
    CHECK_INT_EQ(WM_MATCH_EXACT, rules.items[0].match);
    CHECK(!rules.items[0].above.is_set);
    CHECK_INT_EQ(WM_ANCHOR_NONE, rules.items[1].anchor);
    CHECK(rules.items[1].title == NULL);
    xfce_window_rules_free(&rules);
    g_hash_table_destroy(properties);
}

static const check_case CASES[] = {
    {"reads_every_field", test_reads_every_field},
    {"rules_are_ordered_by_number", test_rules_are_ordered_by_number},
    {"defaults_and_bad_values", test_defaults_and_bad_values},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
