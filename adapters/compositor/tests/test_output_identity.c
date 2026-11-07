#include <glib.h>

#include "check.h"
#include "output/output_identity.h"

static void test_matches_hash_of_names(void)
{
    display_identity identity = { .make = "Acme", .model = "Panel", .serial = "42" };
    char *id = g_compute_checksum_for_string(G_CHECKSUM_SHA1, "42-Panel-Acme", -1);

    CHECK(output_identity_matches(&identity, id));
    g_free(id);
}

static void test_missing_names_hash_like_xfce_settings(void)
{
    display_identity identity = { .make = "Acme" };
    char *id = g_compute_checksum_for_string(G_CHECKSUM_SHA1, "(null)-(null)-Acme", -1);

    CHECK(output_identity_matches(&identity, id));
    g_free(id);
}

static void test_matches_hash_of_edid(void)
{
    static const uint8_t edid[] = { 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 };
    display_identity identity = { .make = "Acme", .edid = edid, .edid_size = sizeof(edid) };
    char *id = g_compute_checksum_for_data(G_CHECKSUM_SHA1, edid, sizeof(edid));
    char *names = g_compute_checksum_for_string(G_CHECKSUM_SHA1, "(null)-(null)-Acme", -1);

    CHECK(output_identity_matches(&identity, id));
    CHECK(output_identity_matches(&identity, names));
    g_free(id);
    g_free(names);
}

static void test_matches_x11_base_block_hash(void)
{
    enum { TV_EDID_SIZE = 256, BASE_BLOCK = 128 };
    uint8_t edid[TV_EDID_SIZE] = { 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 };
    edid[BASE_BLOCK] = 0x02;
    display_identity identity = { .edid = edid, .edid_size = sizeof(edid) };
    char *x11 = g_compute_checksum_for_data(G_CHECKSUM_SHA1, edid, BASE_BLOCK);
    char *full = g_compute_checksum_for_data(G_CHECKSUM_SHA1, edid, sizeof(edid));

    CHECK(output_identity_matches(&identity, x11));
    CHECK(output_identity_matches(&identity, full));
    g_free(x11);
    g_free(full);
}

static void test_rejects_other_and_empty_ids(void)
{
    display_identity identity = { .make = "Acme", .model = "Panel", .serial = "42" };

    CHECK(!output_identity_matches(&identity, "0000000000000000000000000000000000000000"));
    CHECK(!output_identity_matches(&identity, ""));
    CHECK(!output_identity_matches(&identity, NULL));
}

int main(void)
{
    static const check_case cases[] = {
        { "matches_hash_of_names", test_matches_hash_of_names },
        { "missing_names_hash_like_xfce_settings", test_missing_names_hash_like_xfce_settings },
        { "matches_hash_of_edid", test_matches_hash_of_edid },
        { "matches_x11_base_block_hash", test_matches_x11_base_block_hash },
        { "rejects_other_and_empty_ids", test_rejects_other_and_empty_ids },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
