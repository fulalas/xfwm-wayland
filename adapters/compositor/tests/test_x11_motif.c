#include "check.h"
#include "wm/desktop.h"
#include "x11_motif.h"

enum {
    FUNCTIONS = 1,
    DECORATIONS = 2,
};

static void test_missing_hints_allow_everything(void)
{
    CHECK_INT_EQ(WM_CAP_ALL, x11_motif_capabilities(NULL, 0));
}

static void test_no_decorations_drop_the_frame(void)
{
    const uint32_t hints[] = {DECORATIONS, 0, 0, 0, 0};
    CHECK_INT_EQ(WM_CAP_ALL & ~WM_CAP_DECORATED, x11_motif_capabilities(hints, 5));
}

static void test_title_or_border_keeps_the_frame(void)
{
    const uint32_t border[] = {DECORATIONS, 0, 1 << 1, 0, 0};
    const uint32_t title[] = {DECORATIONS, 0, 1 << 3, 0, 0};
    CHECK_INT_EQ(WM_CAP_ALL, x11_motif_capabilities(border, 5));
    CHECK_INT_EQ(WM_CAP_ALL, x11_motif_capabilities(title, 5));
}

static void test_listed_functions_are_the_only_ones(void)
{
    const uint32_t hints[] = {FUNCTIONS, (1 << 2) | (1 << 5), 0, 0, 0};
    uint32_t caps = x11_motif_capabilities(hints, 5);
    CHECK_INT_EQ(WM_CAP_ALL & ~(WM_CAP_RESIZE | WM_CAP_MINIMIZE | WM_CAP_MAXIMIZE), caps);
}

static void test_all_with_listed_functions_removes_them(void)
{
    const uint32_t hints[] = {FUNCTIONS, 1 | (1 << 5) | (1 << 4), 0, 0, 0};
    uint32_t caps = x11_motif_capabilities(hints, 5);
    CHECK_INT_EQ(WM_CAP_ALL & ~(WM_CAP_CLOSE | WM_CAP_MAXIMIZE), caps);
}

int main(void)
{
    static const check_case cases[] = {
        {"missing_hints_allow_everything", test_missing_hints_allow_everything},
        {"no_decorations_drop_the_frame", test_no_decorations_drop_the_frame},
        {"title_or_border_keeps_the_frame", test_title_or_border_keeps_the_frame},
        {"listed_functions_are_the_only_ones", test_listed_functions_are_the_only_ones},
        {"all_with_listed_functions_removes_them", test_all_with_listed_functions_removes_them},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
