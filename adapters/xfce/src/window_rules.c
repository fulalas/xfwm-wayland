#include "window_rules.h"

#include <stddef.h>
#include <string.h>

#include "config_values.h"
#include "value_text.h"

const char window_rules_root[] = "/window-rules";

typedef struct named_rule {
    char *name;
    wm_window_rule rule;
} named_rule;

typedef enum field_kind {
    FIELD_TEXT,
    FIELD_SWITCH,
    FIELD_MATCH,
    FIELD_POSITION,
    FIELD_MARGIN,
    FIELD_WORKSPACE,
} field_kind;

typedef struct rule_field {
    const char *name;
    field_kind kind;
    size_t offset;
} rule_field;

#define AT(member) offsetof(wm_window_rule, member)

static const rule_field rule_fields[] = {
    {"title", FIELD_TEXT, AT(title)},
    {"app_id", FIELD_TEXT, AT(app_id)},
    {"output", FIELD_TEXT, AT(output)},
    {"above", FIELD_SWITCH, AT(above)},
    {"skip_taskbar", FIELD_SWITCH, AT(skip_taskbar)},
    {"no_focus", FIELD_SWITCH, AT(no_focus)},
    {"match", FIELD_MATCH, 0},
    {"position", FIELD_POSITION, 0},
    {"margin", FIELD_MARGIN, 0},
    {"workspace", FIELD_WORKSPACE, 0},
};

static bool split_key(const char *key, char **name, const char **field);
static named_rule *rule_named(GPtrArray *rules, const char *name);
static void apply_field(wm_window_rule *rule, const char *field, const GValue *value);
static void set_field(wm_window_rule *rule, const rule_field *field, const char *text);
static void replace_text(const char **slot, const char *text);
static int compare_names(const void *a, const void *b);
static bool is_number(const char *text);
static void free_named_rule(void *data);
static void free_rule_texts(wm_window_rule *rule);

xfce_window_rules window_rules_from(GHashTable *properties)
{
    GPtrArray *named = g_ptr_array_new_with_free_func(free_named_rule);
    GHashTableIter iter;
    void *key;
    void *value;

    g_hash_table_iter_init(&iter, properties);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        char *name;
        const char *field;
        if (split_key(key, &name, &field)) {
            apply_field(&rule_named(named, name)->rule, field, value);
            g_free(name);
        }
    }
    g_ptr_array_sort(named, compare_names);
    xfce_window_rules rules = {g_new0(wm_window_rule, named->len ? named->len : 1), named->len};
    for (guint i = 0; i < named->len; i++) {
        named_rule *entry = g_ptr_array_index(named, i);
        rules.items[i] = entry->rule;
        memset(&entry->rule, 0, sizeof(entry->rule));
    }
    g_ptr_array_free(named, TRUE);
    return rules;
}

void xfce_window_rules_free(xfce_window_rules *rules)
{
    for (size_t i = 0; i < rules->count; i++) {
        free_rule_texts(&rules->items[i]);
    }
    g_free(rules->items);
    rules->items = NULL;
    rules->count = 0;
}

static bool split_key(const char *key, char **name, const char **field)
{
    size_t root_length = strlen(window_rules_root);
    if (strncmp(key, window_rules_root, root_length) != 0 || key[root_length] != '/') {
        return false;
    }
    const char *start = key + root_length + 1;
    const char *slash = strchr(start, '/');
    if (slash == NULL || slash == start || strchr(slash + 1, '/') != NULL) {
        return false;
    }
    *name = g_strndup(start, (gsize) (slash - start));
    *field = slash + 1;
    return true;
}

static named_rule *rule_named(GPtrArray *rules, const char *name)
{
    for (guint i = 0; i < rules->len; i++) {
        named_rule *entry = g_ptr_array_index(rules, i);
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
    }
    named_rule *entry = g_new0(named_rule, 1);
    entry->name = g_strdup(name);
    g_ptr_array_add(rules, entry);
    return entry;
}

static void apply_field(wm_window_rule *rule, const char *field, const GValue *value)
{
    for (size_t i = 0; i < G_N_ELEMENTS(rule_fields); i++) {
        if (strcmp(rule_fields[i].name, field) == 0) {
            char *text = value_text_of(value);
            if (text != NULL) {
                set_field(rule, &rule_fields[i], text);
            }
            g_free(text);
            return;
        }
    }
}

static void set_field(wm_window_rule *rule, const rule_field *field, const char *text)
{
    void *slot = (char *) rule + field->offset;
    int number = config_int(text);
    switch (field->kind) {
    case FIELD_TEXT:
        replace_text(slot, text);
        break;
    case FIELD_SWITCH:
        *(wm_rule_switch *) slot = (wm_rule_switch) {true, config_bool(text) || number == 1};
        break;
    case FIELD_MATCH:
        rule->match = config_rule_match(text);
        break;
    case FIELD_POSITION:
        rule->anchor = config_rule_anchor(text);
        break;
    case FIELD_MARGIN:
        rule->margin = MAX(number, 0);
        break;
    case FIELD_WORKSPACE:
        rule->has_workspace = number > 0;
        rule->workspace = number - 1;
        break;
    }
}

static void replace_text(const char **slot, const char *text)
{
    g_free((char *) *slot);
    *slot = *text != '\0' ? g_strdup(text) : NULL;
}

static int compare_names(const void *a, const void *b)
{
    const named_rule *left = *(named_rule *const *) a;
    const named_rule *right = *(named_rule *const *) b;
    bool left_number = is_number(left->name);
    bool right_number = is_number(right->name);
    if (left_number && right_number) {
        long long difference = g_ascii_strtoll(left->name, NULL, 10)
                               - g_ascii_strtoll(right->name, NULL, 10);
        return (difference > 0) - (difference < 0);
    }
    if (left_number != right_number) {
        return left_number ? -1 : 1;
    }
    return strcmp(left->name, right->name);
}

static bool is_number(const char *text)
{
    size_t length = strlen(text);
    return length > 0 && length < 18 && strspn(text, "0123456789") == length;
}

static void free_named_rule(void *data)
{
    named_rule *entry = data;
    free_rule_texts(&entry->rule);
    g_free(entry->name);
    g_free(entry);
}

static void free_rule_texts(wm_window_rule *rule)
{
    g_free((char *) rule->title);
    g_free((char *) rule->app_id);
    g_free((char *) rule->output);
}
