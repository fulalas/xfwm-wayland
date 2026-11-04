#ifndef COMPOSITOR_OUTPUT_POWER_H
#define COMPOSITOR_OUTPUT_POWER_H

#include "server.h"

output_power *output_power_create(compositor *server);
void output_power_destroy(output_power *power);

#endif
