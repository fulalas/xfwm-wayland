#ifndef XFCE_ICON_LOOKUPS_H
#define XFCE_ICON_LOOKUPS_H

#include "compositor/services.h"

enum {
    ICON_LOOKUP_KNOWN = 0,
};

typedef struct icon_lookups icon_lookups;

icon_lookups *icon_lookups_create(void);
void icon_lookups_destroy(icon_lookups *lookups);
unsigned icon_lookups_add(icon_lookups *lookups, const icon_request *request);
void icon_lookups_cancel(icon_lookups *lookups, void *context);
void icon_lookups_finish(icon_lookups *lookups, unsigned id, const char *path);
void icon_lookups_forget_paths(icon_lookups *lookups);

#endif
