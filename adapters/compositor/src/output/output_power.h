#ifndef COMPOSITOR_OUTPUT_POWER_H
#define COMPOSITOR_OUTPUT_POWER_H

#include <stdbool.h>

#include "output.h"
#include "server/server.h"

output_power *output_power_create(compositor *server);
void output_power_destroy(output_power *power);
void output_power_set(compositor *server, output *output, bool on);

#endif
