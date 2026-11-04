#include "setting_source.h"

#include <stddef.h>

const char *setting_source_lookup(const setting_source *source, const char *key)
{
    if (!source || !source->lookup) {
        return NULL;
    }
    return source->lookup(source->self, key);
}
