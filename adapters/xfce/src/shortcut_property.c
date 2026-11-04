#include "shortcut_property.h"

#include <stddef.h>
#include <string.h>

const char *shortcut_property_accelerator(const char *property, const char *base)
{
    size_t base_length = strlen(base);
    const char *accelerator;

    if (strncmp(property, base, base_length) != 0 || property[base_length] != '/') {
        return NULL;
    }
    accelerator = property + base_length + 1;
    if (*accelerator == '\0' || strchr(accelerator, '/')) {
        return NULL;
    }
    return accelerator;
}
