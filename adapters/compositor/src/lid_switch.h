#ifndef COMPOSITOR_LID_SWITCH_H
#define COMPOSITOR_LID_SWITCH_H

#include "server.h"

lid_switch *lid_switch_create(compositor *server);
void lid_switch_destroy(lid_switch *lid);

#endif
