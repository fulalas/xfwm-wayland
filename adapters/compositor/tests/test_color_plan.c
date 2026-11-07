#include "check.h"
#include "output/color_plan.h"

static void test_default_is_plain_eight_bit(void)
{
    color_plan plan = color_plan_for((color_mode){0});
    CHECK_INT_EQ(1, (int) plan.count);
    CHECK(!plan.attempts[0].hdr && !plan.attempts[0].ten_bit);
}

static void test_hdr_tries_ten_bit_hdr_then_sdr(void)
{
    color_plan plan = color_plan_for((color_mode){.hdr = true});
    CHECK_INT_EQ(2, (int) plan.count);
    CHECK(plan.attempts[0].hdr && plan.attempts[0].ten_bit);
    CHECK(!plan.attempts[1].hdr && !plan.attempts[1].ten_bit);
}

static void test_hdr_with_ten_bit_keeps_ten_bit_sdr_fallback(void)
{
    color_plan plan = color_plan_for((color_mode){.hdr = true, .ten_bit = true});
    CHECK_INT_EQ(3, (int) plan.count);
    CHECK(!plan.attempts[1].hdr && plan.attempts[1].ten_bit);
}

static void test_ten_bit_alone_falls_back_to_eight_bit(void)
{
    color_plan plan = color_plan_for((color_mode){.ten_bit = true});
    CHECK_INT_EQ(2, (int) plan.count);
    CHECK(!plan.attempts[0].hdr && plan.attempts[0].ten_bit);
}

int main(void)
{
    const check_case cases[] = {
        {"default_is_plain_eight_bit", test_default_is_plain_eight_bit},
        {"hdr_tries_ten_bit_hdr_then_sdr", test_hdr_tries_ten_bit_hdr_then_sdr},
        {"hdr_with_ten_bit_keeps_ten_bit_sdr_fallback",
         test_hdr_with_ten_bit_keeps_ten_bit_sdr_fallback},
        {"ten_bit_alone_falls_back_to_eight_bit", test_ten_bit_alone_falls_back_to_eight_bit},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
