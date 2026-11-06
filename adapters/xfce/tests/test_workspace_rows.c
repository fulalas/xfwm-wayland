#include "check.h"
#include "workspace_rows.h"

static GHashTable *plugins_new(void)
{
    return g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify) g_free);
}

static void add_text(GHashTable *plugins, const char *key, const char *text)
{
    GValue *value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_STRING);
    g_value_set_string(value, text);
    g_hash_table_insert(plugins, (char *) key, value);
}

static void add_rows(GHashTable *plugins, const char *key, unsigned rows)
{
    GValue *value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_UINT);
    g_value_set_uint(value, rows);
    g_hash_table_insert(plugins, (char *) key, value);
}

static void test_no_pager_gives_zero(void)
{
    GHashTable *plugins = plugins_new();
    add_text(plugins, "/plugins/plugin-1", "tasklist");
    add_rows(plugins, "/plugins/plugin-1/rows", 5);
    CHECK_INT_EQ(0, pager_rows_from(plugins));
    g_hash_table_destroy(plugins);
}

static void test_first_pager_rows_win(void)
{
    GHashTable *plugins = plugins_new();
    add_text(plugins, "/plugins/plugin-9", "pager");
    add_rows(plugins, "/plugins/plugin-9/rows", 3);
    add_text(plugins, "/plugins/plugin-4", "pager");
    add_rows(plugins, "/plugins/plugin-4/rows", 2);
    CHECK_INT_EQ(2, pager_rows_from(plugins));
    g_hash_table_destroy(plugins);
}

static void test_pager_without_rows_has_one_row(void)
{
    GHashTable *plugins = plugins_new();
    add_text(plugins, "/plugins/plugin-2", "pager");
    CHECK_INT_EQ(1, pager_rows_from(plugins));
    add_rows(plugins, "/plugins/plugin-2/rows", 0);
    CHECK_INT_EQ(1, pager_rows_from(plugins));
    g_hash_table_destroy(plugins);
}

int main(void)
{
    static const check_case cases[] = {
        { "no_pager_gives_zero", test_no_pager_gives_zero },
        { "first_pager_rows_win", test_first_pager_rows_win },
        { "pager_without_rows_has_one_row", test_pager_without_rows_has_one_row },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
