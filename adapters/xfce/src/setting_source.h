#ifndef XFCE_SETTING_SOURCE_H
#define XFCE_SETTING_SOURCE_H

#include "xfce/environment.h"

typedef xfce_setting_source setting_source;

const char *setting_source_lookup(const setting_source *source, const char *key);

#endif
