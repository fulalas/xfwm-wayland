#ifndef COMPOSITOR_IDLE_H
#define COMPOSITOR_IDLE_H

#include "server/server.h"

idle *idle_create(compositor *server);
void idle_destroy(idle *idle);
void idle_schedule_refresh(idle *idle);
void idle_activity(idle *idle);
void idle_reload_display_off(idle *idle);

#endif
