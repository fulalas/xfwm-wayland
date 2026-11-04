#include "display_hash.h"

#include <glib.h>

static const char missing_text[] = "(null)";

static const char *or_missing(const char *text);

char *display_hash(const display_identity *identity)
{
    char *fallback;
    char *hash;

    if (identity->edid && identity->edid_size > 0) {
        return g_compute_checksum_for_data(G_CHECKSUM_SHA1, identity->edid, identity->edid_size);
    }
    fallback = g_strdup_printf("%s-%s-%s", or_missing(identity->serial),
                               or_missing(identity->model), or_missing(identity->make));
    hash = g_compute_checksum_for_string(G_CHECKSUM_SHA1, fallback, -1);
    g_free(fallback);
    return hash;
}

static const char *or_missing(const char *text)
{
    return text ? text : missing_text;
}
