#include <glib.h>

#include "compositor/displays.h"

static const char missing_text[] = "(null)";

static const char *or_missing(const char *text);

char *display_hash(const display_identity *identity)
{
    char *hash = display_hash_edid(identity);

    return hash != NULL ? hash : display_hash_names(identity);
}

char *display_hash_edid(const display_identity *identity)
{
    if (identity->edid == NULL || identity->edid_size == 0) {
        return NULL;
    }
    return g_compute_checksum_for_data(G_CHECKSUM_SHA1, identity->edid, identity->edid_size);
}

char *display_hash_names(const display_identity *identity)
{
    char *text = g_strdup_printf("%s-%s-%s", or_missing(identity->serial),
                                 or_missing(identity->model), or_missing(identity->make));
    char *hash = g_compute_checksum_for_string(G_CHECKSUM_SHA1, text, -1);

    g_free(text);
    return hash;
}

static const char *or_missing(const char *text)
{
    return text != NULL ? text : missing_text;
}
