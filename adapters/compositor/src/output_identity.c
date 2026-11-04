#include <glib.h>

#include "output_identity.h"

static const char missing_text[] = "(null)";

static bool edid_matches(const display_identity *identity, const char *id);
static bool names_match(const display_identity *identity, const char *id);
static const char *or_missing(const char *text);

bool output_identity_matches(const display_identity *identity, const char *id)
{
    if (id == NULL || id[0] == '\0') {
        return false;
    }
    return edid_matches(identity, id) || names_match(identity, id);
}

static bool edid_matches(const display_identity *identity, const char *id)
{
    if (identity->edid == NULL || identity->edid_size == 0) {
        return false;
    }
    char *hash = g_compute_checksum_for_data(G_CHECKSUM_SHA1, identity->edid, identity->edid_size);
    bool matches = g_ascii_strcasecmp(hash, id) == 0;
    g_free(hash);
    return matches;
}

static bool names_match(const display_identity *identity, const char *id)
{
    char *text = g_strdup_printf("%s-%s-%s", or_missing(identity->serial),
                                 or_missing(identity->model), or_missing(identity->make));
    char *hash = g_compute_checksum_for_string(G_CHECKSUM_SHA1, text, -1);
    bool matches = g_ascii_strcasecmp(hash, id) == 0;
    g_free(hash);
    g_free(text);
    return matches;
}

static const char *or_missing(const char *text)
{
    return text != NULL ? text : missing_text;
}
