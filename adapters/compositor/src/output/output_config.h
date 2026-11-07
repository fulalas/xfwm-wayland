#ifndef COMPOSITOR_OUTPUT_CONFIG_H
#define COMPOSITOR_OUTPUT_CONFIG_H

#include <stdbool.h>

#include "server/server.h"

struct wlr_output_configuration_v1;

output_config *output_config_create(compositor *server);
void output_config_destroy(output_config *config);
void output_config_schedule_publish(output_config *config);

bool output_config_test(compositor *server, const struct wlr_output_configuration_v1 *config);
bool output_config_commit(compositor *server, const struct wlr_output_configuration_v1 *config);

#endif
