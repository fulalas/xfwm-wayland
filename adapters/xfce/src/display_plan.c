#include "display_plan.h"

#include "display_choice.h"
#include "display_hash.h"
#include "display_read.h"

static char **hash_all(const display_set *connected, size_t count);
static void keep_one_primary(display_plan *plan, size_t count);

display_plan display_profile_plan(GHashTable *table, const display_set *connected)
{
    display_plan plan = { 0 };
    size_t count = MIN(connected->count, (size_t) DISPLAY_MAX);
    char **hashes = hash_all(connected, count);
    char *profile = display_profile_choose(table, hashes);

    for (size_t i = 0; i < count; i++) {
        display_lookup lookup = {
            .table = table,
            .profile = profile,
            .connector = connected->displays[i].identity.connector,
            .hash = hashes[i],
        };
        plan.displays[i] = display_profile_read(&lookup);
    }
    keep_one_primary(&plan, count);
    g_free(profile);
    g_strfreev(hashes);
    return plan;
}

static char **hash_all(const display_set *connected, size_t count)
{
    char **hashes = g_new0(char *, count + 1);

    for (size_t i = 0; i < count; i++) {
        hashes[i] = display_hash(&connected->displays[i].identity);
    }
    return hashes;
}

static void keep_one_primary(display_plan *plan, size_t count)
{
    bool seen = false;

    for (size_t i = 0; i < count; i++) {
        plan->displays[i].is_primary = plan->displays[i].is_primary && !seen;
        seen = seen || plan->displays[i].is_primary;
    }
}
