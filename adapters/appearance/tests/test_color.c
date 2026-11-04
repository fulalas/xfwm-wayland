#include "check.h"
#include "color.h"

enum {
    BYTE_SHIFT = 8,
    RED_SHIFT = 16,
    BYTE_MASK = 0xFF,
};

static bool parses_to(const char *spec, unsigned rgb24)
{
    xpm_rgb rgb;

    if (!appearance_xpm_color_parse(spec, &rgb)) {
        return false;
    }
    unsigned bytes = ((unsigned) (rgb.red >> BYTE_SHIFT) << RED_SHIFT)
                     | ((unsigned) (rgb.green >> BYTE_SHIFT) << BYTE_SHIFT)
                     | (unsigned) (rgb.blue >> BYTE_SHIFT);
    return bytes == rgb24;
}

static bool is_rejected(const char *spec)
{
    xpm_rgb rgb;

    return !appearance_xpm_color_parse(spec, &rgb);
}

static void test_hex_one_digit(void)
{
    xpm_rgb rgb;

    CHECK(appearance_xpm_color_parse("#f0a", &rgb));
    CHECK_INT_EQ(0xFFFF, rgb.red);
    CHECK_INT_EQ(0x0000, rgb.green);
    CHECK_INT_EQ(0xAAAA, rgb.blue);
}

static void test_hex_two_digits(void)
{
    xpm_rgb rgb;

    CHECK(appearance_xpm_color_parse("#ff8000", &rgb));
    CHECK_INT_EQ(0x8080, rgb.green);
    CHECK(parses_to("#3584E4", 0x3584E4));
}

static void test_hex_three_digits(void)
{
    CHECK(parses_to("#fff000aaa", 0xFF00AA));
    CHECK(parses_to("#123456789", 0x124578));
}

static void test_hex_four_digits(void)
{
    xpm_rgb rgb;

    CHECK(appearance_xpm_color_parse("#123456789abc", &rgb));
    CHECK_INT_EQ(0x1234, rgb.red);
    CHECK_INT_EQ(0x5678, rgb.green);
    CHECK_INT_EQ(0x9ABC, rgb.blue);
}

static void test_hex_bad_forms_are_rejected(void)
{
    CHECK(is_rejected("#"));
    CHECK(is_rejected("#ff80"));
    CHECK(is_rejected("#1234567890abcde"));
    CHECK(is_rejected("#ggg"));
}

static void test_names_ignore_case(void)
{
    CHECK(parses_to("Red", 0xFF0000));
    CHECK(parses_to("LIGHT GOLDENROD", 0xEEDD82));
    CHECK(parses_to("DarkSlateGray", 0x2F4F4F));
}

static void test_names_with_spaces(void)
{
    CHECK(parses_to("Light Goldenrod", 0xEEDD82));
    CHECK(parses_to("lightgoldenrod", 0xEEDD82));
    CHECK(parses_to("dark slate gray", 0x2F4F4F));
    CHECK(parses_to("light blue", 0xADD8E6));
}

static void test_names_keep_x11_values(void)
{
    CHECK(parses_to("gray", 0xBEBEBE));
    CHECK(parses_to("Grey", 0xBEBEBE));
    CHECK(parses_to("green", 0x00FF00));
    CHECK(parses_to("maroon", 0xB03060));
    CHECK(parses_to("purple", 0xA020F0));
    CHECK(parses_to("gray50", 0x7F7F7F));
}

static void test_none_and_unknown_are_rejected(void)
{
    CHECK(is_rejected("None"));
    CHECK(is_rejected("no such color"));
    CHECK(is_rejected(""));
}

static void test_css_colors(void)
{
    ui_color color;

    CHECK(appearance_color_parse("rgb(255,0,0)", &color));
    CHECK_DOUBLE_NEAR(1.0, color.red, 1e-9);
    CHECK(appearance_color_parse("rgba(0, 0, 255, 0.5)", &color));
    CHECK_DOUBLE_NEAR(0.5, color.alpha, 1e-9);
    CHECK(appearance_color_parse("#000000", &color));
    CHECK_DOUBLE_NEAR(1.0, color.alpha, 1e-9);
}

static void test_named_ui_color(void)
{
    ui_color color;

    CHECK(appearance_color_parse("white", &color));
    CHECK_DOUBLE_NEAR(1.0, color.green, 1e-9);
    CHECK(!appearance_color_parse("None", &color));
}

int main(void)
{
    static const check_case cases[] = {
        { "hex_one_digit", test_hex_one_digit },
        { "hex_two_digits", test_hex_two_digits },
        { "hex_three_digits", test_hex_three_digits },
        { "hex_four_digits", test_hex_four_digits },
        { "hex_bad_forms_are_rejected", test_hex_bad_forms_are_rejected },
        { "names_ignore_case", test_names_ignore_case },
        { "names_with_spaces", test_names_with_spaces },
        { "names_keep_x11_values", test_names_keep_x11_values },
        { "none_and_unknown_are_rejected", test_none_and_unknown_are_rejected },
        { "css_colors", test_css_colors },
        { "named_ui_color", test_named_ui_color },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
