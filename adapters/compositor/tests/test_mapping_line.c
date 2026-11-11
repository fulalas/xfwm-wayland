#include "check.h"
#include "server/mapping_line.h"

static void test_client_memfd_is_released(void)
{
    mapping found = {0};
    CHECK(mapping_line_parse(
        "7f0000001000-7f0000004000 rw-s 00000000 00:01 2053   /memfd:gdk-wayland (deleted)\n",
        &found));
    CHECK(found.start == 0x7f0000001000);
    CHECK_INT_EQ(0x3000, (int) found.length);
}

static void test_shm_file_is_released(void)
{
    mapping found = {0};
    CHECK(mapping_line_parse(
        "7f0000001000-7f0000002000 r--s 00000000 00:19 7   /dev/shm/wayland-shm-1 (deleted)\n",
        &found));
}

static void test_library_code_and_constants_are_released(void)
{
    mapping found = {0};
    CHECK(mapping_line_parse(
        "7f0000001000-7f0000004000 r-xp 00021000 103:03 77   /usr/lib64/libLLVM.so.23.1\n",
        &found));
    CHECK(mapping_line_parse(
        "7f0000001000-7f0000004000 r--p 00021000 103:03 77   /usr/lib64/libLLVM.so.23.1\n",
        &found));
}

static void test_writable_private_mapping_is_skipped(void)
{
    mapping found = {0};
    CHECK(!mapping_line_parse(
        "7f0000001000-7f0000004000 rw-p 00021000 103:03 77   /usr/lib64/libLLVM.so.23.1\n",
        &found));
    CHECK(!mapping_line_parse("55d000000000-55d000021000 rw-p 00000000 00:00 0   [heap]\n",
                              &found));
    CHECK(!mapping_line_parse(
        "7f0000001000-7f0000004000 ---p 00021000 103:03 77   /usr/lib64/libLLVM.so.23.1\n",
        &found));
}

static void test_device_mapping_is_skipped(void)
{
    mapping found = {0};
    CHECK(!mapping_line_parse(
        "7f0000001000-7f0000004000 rw-s 10a3c8000 00:06 380   /dev/dri/renderD128\n", &found));
    CHECK(!mapping_line_parse(
        "7f0000001000-7f0000004000 rw-s 00000000 00:01 9   /dev/zero (deleted)\n", &found));
}

static void test_nameless_or_broken_line_is_skipped(void)
{
    mapping found = {0};
    CHECK(!mapping_line_parse("7f0000001000-7f0000004000 rw-s 00000000 00:00 0 \n", &found));
    CHECK(!mapping_line_parse("7f0000001000-7f0000004000 r--p 00000000 00:00 0   [vvar]\n",
                              &found));
    CHECK(!mapping_line_parse("7f0000001000-7f0000004000 rw-s 00000000 00:0e 11   "
                              "anon_inode:dmabuf\n",
                              &found));
    CHECK(!mapping_line_parse("wayland (deleted)\n", &found));
    CHECK(!mapping_line_parse("", &found));
}

int main(void)
{
    const check_case cases[] = {
        {"client_memfd_is_released", test_client_memfd_is_released},
        {"shm_file_is_released", test_shm_file_is_released},
        {"library_code_and_constants_are_released",
         test_library_code_and_constants_are_released},
        {"writable_private_mapping_is_skipped", test_writable_private_mapping_is_skipped},
        {"device_mapping_is_skipped", test_device_mapping_is_skipped},
        {"nameless_or_broken_line_is_skipped", test_nameless_or_broken_line_is_skipped},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
