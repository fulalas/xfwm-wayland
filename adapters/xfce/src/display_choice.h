#ifndef XFCE_DISPLAY_CHOICE_H
#define XFCE_DISPLAY_CHOICE_H

#include <glib.h>

extern const char display_default_profile[];

char *display_profile_choose(GHashTable *table, char **const *displays);
char *display_profile_output_name(const char *key, const char *profile);

#endif
