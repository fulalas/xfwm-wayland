#ifndef XFCE_ACCELERATOR_H
#define XFCE_ACCELERATOR_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/keys.h"

bool accelerator_parse(const char *text, wm_key *key);
uint32_t accelerator_modifier_from_name(const char *name);

#endif
