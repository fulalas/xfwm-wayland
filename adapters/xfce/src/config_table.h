#ifndef XFCE_CONFIG_TABLE_H
#define XFCE_CONFIG_TABLE_H

#include "setting_source.h"
#include "wm/config.h"

typedef enum config_type {
    CONFIG_TEXT,
    CONFIG_INT,
    CONFIG_BOOL,
} config_type;

typedef struct config_default {
    const char *name;
    config_type type;
    const char *text;
} config_default;

typedef struct config_default_visitor {
    void *self;
    void (*visit)(void *self, const config_default *entry);
} config_default_visitor;

typedef struct config_sources {
    setting_source stored;
    setting_source theme;
    setting_source xsettings;
} config_sources;

void config_apply_sources(wm_config *config, const config_sources *sources);
void config_visit_defaults(const config_default_visitor *visitor);
const char *config_default_text(const char *name);
void config_set_workspace_names(wm_config *config, const char *const *names);

#endif
