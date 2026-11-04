#include "check.h"
#include "mode_choice.h"

static const mode_option modes[] = {
    {{3840, 2160, 60000}, true},
    {{3840, 2160, 30000}, false},
    {{2560, 1440, 59951}, false},
    {{2560, 1440, 143912}, false},
    {{1920, 1080, 60000}, false},
};

static int pick(int width, int height, int refresh_mhz)
{
    mode_options options = {modes, sizeof(modes) / sizeof(modes[0])};
    display_mode wanted = {width, height, refresh_mhz};
    return mode_choice_pick(&options, &wanted);
}

static void test_exact_mode(void)
{
    CHECK_INT_EQ(2, pick(2560, 1440, 59951));
    CHECK_INT_EQ(3, pick(2560, 1440, 143912));
}

static void test_nearest_refresh_for_size(void)
{
    CHECK_INT_EQ(2, pick(2560, 1440, 59950));
    CHECK_INT_EQ(3, pick(2560, 1440, 120000));
}

static void test_highest_refresh_without_rate(void)
{
    CHECK_INT_EQ(3, pick(2560, 1440, 0));
}

static void test_unknown_size(void)
{
    CHECK_INT_EQ(MODE_CHOICE_NONE, pick(1280, 1024, 60000));
}

static void test_no_size_gives_preferred(void)
{
    CHECK_INT_EQ(0, pick(0, 0, 0));
}

static void test_empty_list(void)
{
    mode_options options = {NULL, 0};
    display_mode wanted = {0, 0, 0};
    CHECK_INT_EQ(MODE_CHOICE_NONE, mode_choice_pick(&options, &wanted));
}

int main(void)
{
    const check_case cases[] = {
        {"exact_mode", test_exact_mode},
        {"nearest_refresh_for_size", test_nearest_refresh_for_size},
        {"highest_refresh_without_rate", test_highest_refresh_without_rate},
        {"unknown_size", test_unknown_size},
        {"no_size_gives_preferred", test_no_size_gives_preferred},
        {"empty_list", test_empty_list},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
