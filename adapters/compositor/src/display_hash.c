#include <glib.h>

#include "compositor/displays.h"

enum {
    EDID_BASE_BLOCK = 128,
};

static const char missing_text[] = "(null)";

static char *edid_hash(const display_identity *identity, size_t size);
static char *names_hash(const display_identity *identity);
static const char *or_missing(const char *text);

char *display_hash(const display_identity *identity)
{
    if (identity->edid == NULL || identity->edid_size == 0) {
        return names_hash(identity);
    }
    return edid_hash(identity, MIN(identity->edid_size, (size_t) EDID_BASE_BLOCK));
}

char **display_hash_candidates(const display_identity *identity)
{
    GPtrArray *hashes = g_ptr_array_new();

    g_ptr_array_add(hashes, display_hash(identity));
    if (identity->edid != NULL && identity->edid_size > EDID_BASE_BLOCK) {
        g_ptr_array_add(hashes, edid_hash(identity, identity->edid_size));
    }
    if (identity->edid != NULL && identity->edid_size > 0) {
        g_ptr_array_add(hashes, names_hash(identity));
    }
    g_ptr_array_add(hashes, NULL);
    return (char **) g_ptr_array_free(hashes, FALSE);
}

bool display_hash_matches(const display_identity *identity, const char *saved)
{
    char **hashes = display_hash_candidates(identity);
    bool matches = false;

    for (size_t i = 0; hashes[i] != NULL && !matches; i++) {
        matches = g_ascii_strcasecmp(hashes[i], saved) == 0;
    }
    g_strfreev(hashes);
    return matches;
}

static char *edid_hash(const display_identity *identity, size_t size)
{
    return g_compute_checksum_for_data(G_CHECKSUM_SHA1, identity->edid, size);
}

static char *names_hash(const display_identity *identity)
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
