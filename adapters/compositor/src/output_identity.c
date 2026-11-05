#include <glib.h>

#include "output_identity.h"

static bool hash_matches(char *hash, const char *id);

bool output_identity_matches(const display_identity *identity, const char *id)
{
    if (id == NULL || id[0] == '\0') {
        return false;
    }
    return hash_matches(display_hash_edid(identity), id)
           || hash_matches(display_hash_names(identity), id);
}

static bool hash_matches(char *hash, const char *id)
{
    bool matches = hash != NULL && g_ascii_strcasecmp(hash, id) == 0;

    g_free(hash);
    return matches;
}
