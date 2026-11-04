#include <string.h>

#include "check.h"
#include "config_table.h"
#include "config_values.h"
#include "fake_source.h"
#include "wm/keys.h"

static const fake_entry no_entries[] = { { NULL, NULL } };

static wm_config apply(const fake_entry *stored, const fake_entry *theme)
{
    config_sources sources = {
        .stored = fake_source(stored),
        .theme = fake_source(theme),
        .xsettings = fake_source(no_entries),
    };
    wm_config config;

    memset(&config, 0, sizeof(config));
    config_apply_sources(&config, &sources);
    return config;
}

static void test_activate_action(void)
{
    CHECK_INT_EQ(WM_ACTIVATE_BRING, config_activate_action("bring"));
    CHECK_INT_EQ(WM_ACTIVATE_SWITCH, config_activate_action("Switch"));
    CHECK_INT_EQ(WM_ACTIVATE_NONE, config_activate_action("none"));
    CHECK_INT_EQ(WM_ACTIVATE_NONE, config_activate_action("garbage"));
}

static void test_double_click_action(void)
{
    CHECK_INT_EQ(WM_DOUBLE_CLICK_MAXIMIZE, config_double_click_action("maximize"));
    CHECK_INT_EQ(WM_DOUBLE_CLICK_SHADE, config_double_click_action("shade"));
    CHECK_INT_EQ(WM_DOUBLE_CLICK_FILL, config_double_click_action("fill"));
    CHECK_INT_EQ(WM_DOUBLE_CLICK_ABOVE, config_double_click_action("above"));
    CHECK_INT_EQ(WM_DOUBLE_CLICK_HIDE, config_double_click_action("hide"));
    CHECK_INT_EQ(WM_DOUBLE_CLICK_NONE, config_double_click_action("none"));
}

static void test_placement_and_alignment(void)
{
    CHECK_INT_EQ(WM_PLACE_MOUSE, config_placement_mode("mouse"));
    CHECK_INT_EQ(WM_PLACE_CENTER, config_placement_mode("center"));
    CHECK_INT_EQ(WM_ALIGN_LEFT, config_alignment("left"));
    CHECK_INT_EQ(WM_ALIGN_RIGHT, config_alignment("RIGHT"));
    CHECK_INT_EQ(WM_ALIGN_CENTER, config_alignment("center"));
}

static void test_title_shadow(void)
{
    CHECK_INT_EQ(WM_TITLE_SHADOW_NONE, config_title_shadow("false"));
    CHECK_INT_EQ(WM_TITLE_SHADOW_UNDER, config_title_shadow("true"));
    CHECK_INT_EQ(WM_TITLE_SHADOW_UNDER, config_title_shadow("under"));
    CHECK_INT_EQ(WM_TITLE_SHADOW_FRAME, config_title_shadow("frame"));
}

static void test_vblank_mode(void)
{
    CHECK_INT_EQ(WM_VBLANK_AUTO, config_vblank_mode("auto"));
    CHECK_INT_EQ(WM_VBLANK_OFF, config_vblank_mode("off"));
    CHECK_INT_EQ(WM_VBLANK_ADAPTIVE, config_vblank_mode("adaptive"));
    CHECK_INT_EQ(WM_VBLANK_AUTO, config_vblank_mode("glx"));
}

static void test_easy_click(void)
{
    CHECK_INT_EQ(WM_MOD_ALT, config_easy_click("Alt"));
    CHECK_INT_EQ(WM_MOD_ALT, config_easy_click("true"));
    CHECK_INT_EQ(WM_MOD_CTRL, config_easy_click("Control"));
    CHECK_INT_EQ(WM_MOD_SHIFT, config_easy_click("Shift"));
    CHECK_INT_EQ(WM_MOD_SUPER, config_easy_click("Super"));
    CHECK_INT_EQ(WM_MOD_SUPER, config_easy_click("Mod4"));
    CHECK_INT_EQ(0, config_easy_click("none"));
}

static void test_bool_and_int_values(void)
{
    static const fake_entry stored[] = {
        { "box_move", "true" }, { "click_to_focus", "FALSE" }, { "snap_width", "17" },
        { "shadow_delta_y", "-3" }, { NULL, NULL },
    };
    wm_config config = apply(stored, no_entries);

    CHECK(config.move.box_move);
    CHECK(!config.focus.click_to_focus);
    CHECK_INT_EQ(17, config.move.snap_width);
    CHECK_INT_EQ(-3, config.compositing.shadow_delta_y);
}

static void test_clamped_values(void)
{
    static const fake_entry stored[] = {
        { "frame_opacity", "150" }, { "inactive_opacity", "-5" }, { "margin_top", "-20" },
        { "workspace_count", "0" }, { "cycle_tabwin_mode", "7" }, { "double_click_time", "-300" },
        { NULL, NULL },
    };
    wm_config config = apply(stored, no_entries);

    CHECK_INT_EQ(100, config.compositing.frame_opacity_percent);
    CHECK_INT_EQ(0, config.compositing.inactive_opacity_percent);
    CHECK_INT_EQ(0, config.placement.margins.top);
    CHECK_INT_EQ(1, config.workspaces.count);
    CHECK_INT_EQ(WM_TABWIN_LIST, config.cycle.tabwin_mode);
    CHECK_INT_EQ(300, config.mouse.double_click_time_ms);
}

static void test_enum_strings_mapped(void)
{
    static const fake_entry stored[] = {
        { "placement_mode", "mouse" }, { "title_alignment", "left" },
        { "easy_click", "Super" }, { "title_shadow_active", "frame" }, { NULL, NULL },
    };
    wm_config config = apply(stored, no_entries);

    CHECK_INT_EQ(WM_PLACE_MOUSE, config.placement.mode);
    CHECK_INT_EQ(WM_ALIGN_LEFT, config.frame.title.alignment);
    CHECK_INT_EQ(WM_MOD_SUPER, config.mouse.easy_click_modifiers);
    CHECK_INT_EQ(WM_TITLE_SHADOW_FRAME, config.frame.title.shadow_active);
}

static void test_theme_overrides_stored(void)
{
    static const fake_entry stored[] = {
        { "button_layout", "O|SHMC" }, { "button_offset", "2" }, { "box_move", "true" },
        { NULL, NULL },
    };
    static const fake_entry theme[] = {
        { "button_layout", "CMH|O" }, { "button_offset", "5" }, { NULL, NULL },
    };
    wm_config config = apply(stored, theme);

    CHECK(strcmp(config.frame.button_layout, "CMH|O") == 0);
    CHECK_INT_EQ(5, config.frame.button_offset);
    CHECK(config.move.box_move);
}

static void test_missing_keys_keep_start_values(void)
{
    config_sources sources = {
        .stored = fake_source(no_entries),
        .theme = { .self = NULL, .lookup = NULL },
        .xsettings = fake_source(no_entries),
    };
    wm_config config;

    memset(&config, 0, sizeof(config));
    config.move.snap_width = 42;
    config_apply_sources(&config, &sources);
    CHECK_INT_EQ(42, config.move.snap_width);
}

static void test_xsettings_double_click_wins(void)
{
    static const fake_entry stored[] = { { "double_click_time", "250" }, { NULL, NULL } };
    static const fake_entry theme[] = { { "double_click_distance", "5" }, { NULL, NULL } };
    static const fake_entry xsettings[] = {
        { "Net/DoubleClickTime", "400" }, { "Net/DoubleClickDistance", "-8" }, { NULL, NULL },
    };
    config_sources sources = {
        .stored = fake_source(stored),
        .theme = fake_source(theme),
        .xsettings = fake_source(xsettings),
    };
    wm_config config;

    memset(&config, 0, sizeof(config));
    config_apply_sources(&config, &sources);
    CHECK_INT_EQ(400, config.mouse.double_click_time_ms);
    CHECK_INT_EQ(8, config.mouse.double_click_distance);
}

static void test_button_layout_truncated(void)
{
    static const fake_entry stored[] = {
        { "button_layout", "OOOOOOOOOOOOOOOOOOOOOOOO|C" }, { NULL, NULL },
    };
    wm_config config = apply(stored, no_entries);

    CHECK_INT_EQ(WM_BUTTON_LAYOUT_MAX - 1, strlen(config.frame.button_layout));
}

static void test_workspace_names(void)
{
    const char *names[] = { "Web", "Mail", NULL };
    wm_config config;

    memset(&config, 0, sizeof(config));
    strcpy(config.workspaces.names[2], "Kept");
    config_set_workspace_names(&config, names);
    CHECK(strcmp(config.workspaces.names[0], "Web") == 0);
    CHECK(strcmp(config.workspaces.names[1], "Mail") == 0);
    CHECK(strcmp(config.workspaces.names[2], "Kept") == 0);
}

static void test_defaults_table(void)
{
    CHECK(strcmp(config_default_text("theme"), "Default") == 0);
    CHECK(strcmp(config_default_text("title_font"), "Sans Bold 9") == 0);
    CHECK(strcmp(config_default_text("button_layout"), "O|SHMC") == 0);
    CHECK(config_default_text("no_such_key") == NULL);
}

typedef struct default_count {
    int stored;
    int texts;
    bool has_margin;
} default_count;

static void count_default(void *self, const config_default *entry)
{
    default_count *count = self;

    count->stored++;
    count->texts += entry->type == CONFIG_TEXT;
    count->has_margin |= strcmp(entry->name, "margin_top") == 0;
}

static void test_defaults_visit_stored_keys_only(void)
{
    default_count count = { 0 };
    config_default_visitor visitor = { .self = &count, .visit = count_default };

    config_visit_defaults(&visitor);
    CHECK_INT_EQ(81, count.stored);
    CHECK_INT_EQ(11, count.texts);
    CHECK(!count.has_margin);
}

int main(void)
{
    static const check_case cases[] = {
        { "activate_action", test_activate_action },
        { "double_click_action", test_double_click_action },
        { "placement_and_alignment", test_placement_and_alignment },
        { "title_shadow", test_title_shadow },
        { "vblank_mode", test_vblank_mode },
        { "easy_click", test_easy_click },
        { "bool_and_int_values", test_bool_and_int_values },
        { "clamped_values", test_clamped_values },
        { "enum_strings_mapped", test_enum_strings_mapped },
        { "theme_overrides_stored", test_theme_overrides_stored },
        { "missing_keys_keep_start_values", test_missing_keys_keep_start_values },
        { "xsettings_double_click_wins", test_xsettings_double_click_wins },
        { "button_layout_truncated", test_button_layout_truncated },
        { "workspace_names", test_workspace_names },
        { "defaults_table", test_defaults_table },
        { "defaults_visit_stored_keys_only", test_defaults_visit_stored_keys_only },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
