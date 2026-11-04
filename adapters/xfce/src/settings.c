#include "settings.h"

#include "config_table.h"
#include "config_values.h"
#include "xfconf_source.h"

static const char general_prefix[] = "/general/";
static const char workspace_names_property[] = "/general/workspace_names";

static void apply_workspace_names(wm_config *config, XfconfChannel *xfwm4);
static void store_default(void *self, const config_default *entry);
static void init_default_value(GValue *value, const config_default *entry);

wm_config settings_load(const channels *channels, const setting_source *theme)
{
    XfconfChannel *xfwm4 = channels_get(channels, CHANNEL_XFWM4);
    xfconf_source *stored = xfconf_source_create(xfwm4, general_prefix);
    xfconf_source *xsettings = xfconf_source_create(channels_get(channels, CHANNEL_XSETTINGS), "/");
    config_sources sources = {
        .stored = xfconf_source_view(stored),
        .theme = theme ? *theme : (setting_source) { 0 },
        .xsettings = xfconf_source_view(xsettings),
    };
    wm_config config = wm_config_defaults();

    config_apply_sources(&config, &sources);
    apply_workspace_names(&config, xfwm4);
    xfconf_source_destroy(stored);
    xfconf_source_destroy(xsettings);
    return config;
}

void settings_store_missing_defaults(XfconfChannel *xfwm4)
{
    config_default_visitor visitor = { .self = xfwm4, .visit = store_default };

    if (xfwm4) {
        config_visit_defaults(&visitor);
    }
}

char *settings_read_text(XfconfChannel *xfwm4, const char *name)
{
    char *property = g_strconcat(general_prefix, name, NULL);
    char *text = NULL;

    if (xfwm4) {
        text = xfconf_channel_get_string(xfwm4, property, NULL);
    }
    g_free(property);
    if (!text || *text == '\0') {
        g_free(text);
        text = g_strdup(config_default_text(name));
    }
    return text;
}

void settings_store_workspace_count(XfconfChannel *xfwm4, int count)
{
    if (!xfwm4) {
        return;
    }
    if (!xfconf_channel_set_int(xfwm4, "/general/workspace_count", count)) {
        g_warning("Cannot store the workspace count %d in xfconf", count);
    }
}

static void apply_workspace_names(wm_config *config, XfconfChannel *xfwm4)
{
    char **names;

    if (!xfwm4) {
        return;
    }
    names = xfconf_channel_get_string_list(xfwm4, workspace_names_property);
    if (names) {
        config_set_workspace_names(config, (const char *const *) names);
    }
    g_strfreev(names);
}

static void store_default(void *self, const config_default *entry)
{
    XfconfChannel *xfwm4 = self;
    char *property = g_strconcat(general_prefix, entry->name, NULL);
    GValue value = G_VALUE_INIT;

    if (!xfconf_channel_has_property(xfwm4, property)) {
        init_default_value(&value, entry);
        xfconf_channel_set_property(xfwm4, property, &value);
        g_value_unset(&value);
    }
    g_free(property);
}

static void init_default_value(GValue *value, const config_default *entry)
{
    switch (entry->type) {
    case CONFIG_INT:
        g_value_init(value, G_TYPE_INT);
        g_value_set_int(value, config_int(entry->text));
        break;
    case CONFIG_BOOL:
        g_value_init(value, G_TYPE_BOOLEAN);
        g_value_set_boolean(value, config_bool(entry->text));
        break;
    default:
        g_value_init(value, G_TYPE_STRING);
        g_value_set_string(value, entry->text);
        break;
    }
}
