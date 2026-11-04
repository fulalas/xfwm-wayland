#ifndef APPEARANCE_THEMERC_H
#define APPEARANCE_THEMERC_H

#include <stdbool.h>

typedef struct themerc themerc;

themerc *appearance_themerc_create(void);
void appearance_themerc_destroy(themerc *rc);
bool appearance_themerc_parse_file(themerc *rc, const char *path);
void appearance_themerc_set(themerc *rc, const char *key, const char *value);
const char *appearance_themerc_get(const themerc *rc, const char *key);

#endif
