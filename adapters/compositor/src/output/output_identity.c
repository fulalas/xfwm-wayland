#include "output_identity.h"

bool output_identity_matches(const display_identity *identity, const char *id)
{
    return id != NULL && id[0] != '\0' && display_hash_matches(identity, id);
}
