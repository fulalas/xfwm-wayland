#include "display_policy.h"

#include <string.h>

static const char *const internal_prefixes[] = { "eDP", "LVDS", "DSI" };

static bool any_external_enabled(const display_wishes *wishes, const bool *enabled);
static bool any_enabled(size_t count, const bool *enabled);
static size_t fallback_index(const display_wishes *wishes);

bool display_is_internal(const char *connector)
{
    size_t count = sizeof(internal_prefixes) / sizeof(internal_prefixes[0]);
    for (size_t i = 0; connector != NULL && i < count; i++) {
        if (strncmp(connector, internal_prefixes[i], strlen(internal_prefixes[i])) == 0) {
            return true;
        }
    }
    return false;
}

void display_policy_decide(const display_wishes *wishes, bool *enabled)
{
    for (size_t i = 0; i < wishes->count; i++) {
        enabled[i] = wishes->wishes[i].wants_enabled;
    }
    if (wishes->lid_closed && any_external_enabled(wishes, enabled)) {
        for (size_t i = 0; i < wishes->count; i++) {
            enabled[i] = enabled[i] && !wishes->wishes[i].is_internal;
        }
    }
    if (wishes->count > 0 && !any_enabled(wishes->count, enabled)) {
        enabled[fallback_index(wishes)] = true;
    }
}

static bool any_external_enabled(const display_wishes *wishes, const bool *enabled)
{
    for (size_t i = 0; i < wishes->count; i++) {
        if (enabled[i] && !wishes->wishes[i].is_internal) {
            return true;
        }
    }
    return false;
}

static bool any_enabled(size_t count, const bool *enabled)
{
    for (size_t i = 0; i < count; i++) {
        if (enabled[i]) {
            return true;
        }
    }
    return false;
}

static size_t fallback_index(const display_wishes *wishes)
{
    for (size_t i = 0; i < wishes->count; i++) {
        if (wishes->wishes[i].is_internal != wishes->lid_closed) {
            return i;
        }
    }
    return 0;
}
