#ifndef XFCE_LOGOUT_POLICY_H
#define XFCE_LOGOUT_POLICY_H

#include <stdbool.h>
#include <stdint.h>

bool logout_manager_unavailable(const char *remote_error);
bool logout_waits_for_state(uint32_t manager_state);

#endif
