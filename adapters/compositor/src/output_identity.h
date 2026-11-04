#ifndef COMPOSITOR_OUTPUT_IDENTITY_H
#define COMPOSITOR_OUTPUT_IDENTITY_H

#include <stdbool.h>

#include "compositor/displays.h"

bool output_identity_matches(const display_identity *identity, const char *id);

#endif
