#ifndef XFCE_INPUT_TEXT_H
#define XFCE_INPUT_TEXT_H

#include <stddef.h>

#include "compositor/services.h"

char *input_device_property_prefix(const char *device_name);
void input_join_options(const char *const *parts, char *joined, size_t size);
optional_bool input_bool_from_text(const char *text);
optional_double input_double_from_text(const char *text);

#endif
