#ifndef XFCE_CHANNELS_H
#define XFCE_CHANNELS_H

#include <xfconf/xfconf.h>

#include "changes.h"

typedef struct channel_watch {
    void *self;
    void (*changed)(void *self, channel_id id, const char *property);
} channel_watch;

typedef struct channels channels;

channels *channels_create(const channel_watch *watch);
void channels_destroy(channels *channels);
XfconfChannel *channels_get(const channels *channels, channel_id id);

#endif
