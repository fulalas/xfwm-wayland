#include <string.h>

#include <glib.h>

#include "check.h"
#include "color_symbols.h"

enum {
    ACTIVE_TEXT_COLOR = 0,
    ACTIVE_COLOR_1 = 6,
    ACTIVE_TEXT_COLOR_2 = 10,
    ACTIVE_COLOR_2 = 11,
    ACTIVE_HILIGHT_2 = 12,
    INACTIVE_MID_2 = 23,
};

static ui_style numbered_style(void)
{
    ui_style style = appearance_fallback_style();

    for (int i = 0; i < APPEARANCE_COLOR_SYMBOL_COUNT; i++) {
        g_snprintf(style.color_symbols[i], APPEARANCE_COLOR_TEXT_MAX, "#0000%02x", i);
    }
    return style;
}

static void test_names_follow_xfwm4_order(void)
{
    CHECK(strcmp(appearance_color_symbol_name(0), "active_text_color") == 0);
    CHECK(strcmp(appearance_color_symbol_name(3), "inactive_text_shadow_color") == 0);
    CHECK(strcmp(appearance_color_symbol_name(9), "active_mid_1") == 0);
    CHECK(strcmp(appearance_color_symbol_name(19), "inactive_text_color_2") == 0);
    CHECK(strcmp(appearance_color_symbol_name(INACTIVE_MID_2), "inactive_mid_2") == 0);
    CHECK(appearance_color_symbol_name(-1) == NULL);
    CHECK(appearance_color_symbol_name(APPEARANCE_COLOR_SYMBOL_COUNT) == NULL);
}

static void test_values_come_from_style(void)
{
    ui_style style = numbered_style();
    color_symbol_values values;

    appearance_color_symbols_resolve(&values, &style, NULL);
    CHECK(strcmp(values.value[ACTIVE_COLOR_1], "#000006") == 0);
    CHECK(strcmp(values.items[ACTIVE_COLOR_1].name, "active_color_1") == 0);
    CHECK(values.items[ACTIVE_COLOR_1].value == values.value[ACTIVE_COLOR_1]);
}

static void test_empty_style_values_use_fallback(void)
{
    ui_style style = numbered_style();
    ui_style fallback = appearance_fallback_style();
    color_symbol_values values;

    style.color_symbols[ACTIVE_COLOR_1][0] = '\0';
    appearance_color_symbols_resolve(&values, &style, NULL);
    CHECK(strcmp(values.value[ACTIVE_COLOR_1], fallback.color_symbols[ACTIVE_COLOR_1]) == 0);
}

static void test_themerc_overrides_style(void)
{
    ui_style style = numbered_style();
    themerc *rc = appearance_themerc_create();
    color_symbol_values values;

    appearance_themerc_set(rc, "Active_Color_1", "#abcdef");
    appearance_color_symbols_resolve(&values, &style, rc);
    CHECK(strcmp(values.value[ACTIVE_COLOR_1], "#abcdef") == 0);
    appearance_themerc_destroy(rc);
}

static void test_references_resolve_transitively(void)
{
    ui_style style = numbered_style();
    themerc *rc = appearance_themerc_create();
    color_symbol_values values;

    appearance_themerc_set(rc, "active_text_color", "active_hilight_2");
    appearance_themerc_set(rc, "active_text_color_2", "ACTIVE_TEXT_COLOR");
    appearance_color_symbols_resolve(&values, &style, rc);
    CHECK(strcmp(values.value[ACTIVE_TEXT_COLOR], "#00000c") == 0);
    CHECK(strcmp(values.value[ACTIVE_TEXT_COLOR_2], "#00000c") == 0);
    appearance_themerc_destroy(rc);
}

static void test_references_use_single_ordered_pass(void)
{
    ui_style style = numbered_style();
    themerc *rc = appearance_themerc_create();
    color_symbol_values values;

    appearance_themerc_set(rc, "active_text_color", "active_hilight_2");
    appearance_themerc_set(rc, "active_hilight_2", "active_color_2");
    appearance_color_symbols_resolve(&values, &style, rc);
    CHECK(strcmp(values.value[ACTIVE_TEXT_COLOR], "active_color_2") == 0);
    CHECK(strcmp(values.value[ACTIVE_HILIGHT_2], "#00000b") == 0);
    CHECK(strcmp(values.value[ACTIVE_COLOR_2], "#00000b") == 0);
    appearance_themerc_destroy(rc);
}

static void test_fallback_style_has_every_symbol(void)
{
    ui_style style = appearance_fallback_style();

    for (int i = 0; i < APPEARANCE_COLOR_SYMBOL_COUNT; i++) {
        CHECK(style.color_symbols[i][0] == '#');
    }
    CHECK(style.font[0] != '\0');
    CHECK_DOUBLE_NEAR(1.0, style.background.alpha, 1e-9);
}

int main(void)
{
    static const check_case cases[] = {
        { "names_follow_xfwm4_order", test_names_follow_xfwm4_order },
        { "values_come_from_style", test_values_come_from_style },
        { "empty_style_values_use_fallback", test_empty_style_values_use_fallback },
        { "themerc_overrides_style", test_themerc_overrides_style },
        { "references_resolve_transitively", test_references_resolve_transitively },
        { "references_use_single_ordered_pass", test_references_use_single_ordered_pass },
        { "fallback_style_has_every_symbol", test_fallback_style_has_every_symbol },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
