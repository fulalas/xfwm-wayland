#ifndef XFCE_POWER_SOURCE_H
#define XFCE_POWER_SOURCE_H

#include <stdbool.h>

typedef struct power_source power_source;

typedef struct power_source_listener {
    void *self;
    void (*changed)(void *self);
} power_source_listener;

power_source *power_source_create(const power_source_listener *listener);
void power_source_destroy(power_source *source);
bool power_source_on_battery(const power_source *source);
bool power_source_manager_running(const power_source *source);

#endif
