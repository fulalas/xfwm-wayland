#ifndef XFCE_SETTINGS_H
#define XFCE_SETTINGS_H

#include <xfconf/xfconf.h>

#include "channels.h"
#include "setting_source.h"
#include "wm/config.h"
#include "xfce/environment.h"

wm_config settings_load(const channels *channels, const setting_source *theme);
void settings_store_missing_defaults(XfconfChannel *xfwm4);
char *settings_read_text(XfconfChannel *xfwm4, const char *name);
void settings_store_workspace_count(XfconfChannel *xfwm4, int count);
void settings_store_workspace_rows(XfconfChannel *xfwm4, int rows);
void settings_store_workspace_names(XfconfChannel *xfwm4, const char *const *names, size_t count);

#endif
