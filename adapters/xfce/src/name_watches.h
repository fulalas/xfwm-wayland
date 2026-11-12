#ifndef XFCE_NAME_WATCHES_H
#define XFCE_NAME_WATCHES_H

#include <gio/gio.h>

GHashTable *name_watches_new(void);
void name_watches_add(GHashTable *watches, GDBusConnection *connection, const char *name,
                      GBusNameVanishedCallback vanished, gpointer data);

#endif
