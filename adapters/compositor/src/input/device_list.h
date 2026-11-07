#ifndef COMPOSITOR_DEVICE_LIST_H
#define COMPOSITOR_DEVICE_LIST_H

#include "server/server.h"

typedef struct device_list device_list;

device_list *device_list_create(compositor *server);
void device_list_destroy(device_list *list);
void device_list_refresh(device_list *list);

#endif
