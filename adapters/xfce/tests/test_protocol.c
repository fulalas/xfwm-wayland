#include <glib.h>
#include <string.h>

#include "changes.h"
#include "check.h"
#include "helper_protocol.h"
#include "style_state.h"

static helper_message message;

static bool parse(const char *line)
{
    return helper_message_parse(line, &message);
}

static void test_parse_symbol(void)
{
    CHECK(parse("symbol\tactive_text_color\t#ffffffffffff"));
    CHECK_INT_EQ(HELPER_MESSAGE_SYMBOL, message.kind);
    CHECK(strcmp(message.name, "active_text_color") == 0);
    CHECK(strcmp(message.text, "#ffffffffffff") == 0);
}

static void test_parse_style_markers_and_font(void)
{
    CHECK(parse("style-begin"));
    CHECK_INT_EQ(HELPER_MESSAGE_STYLE_BEGIN, message.kind);
    CHECK(parse("font\tCantarell 11"));
    CHECK_INT_EQ(HELPER_MESSAGE_FONT, message.kind);
    CHECK(strcmp(message.text, "Cantarell 11") == 0);
    CHECK(parse("style-end"));
    CHECK_INT_EQ(HELPER_MESSAGE_STYLE_END, message.kind);
}

static void test_parse_icon_reply(void)
{
    CHECK(parse("icon\t42\t/usr/share/icons/a\\tb.png"));
    CHECK_INT_EQ(HELPER_MESSAGE_ICON, message.kind);
    CHECK_INT_EQ(42, message.id);
    CHECK(strcmp(message.text, "/usr/share/icons/a\tb.png") == 0);
    CHECK(parse("icon\t7\t"));
    CHECK(strcmp(message.text, "") == 0);
}

static void test_parse_rejects_bad_lines(void)
{
    CHECK(!parse(""));
    CHECK(!parse("bogus\tx"));
    CHECK(!parse("symbol\tonly_name"));
    CHECK(!parse("icon\tabc\t/path"));
    CHECK(!parse("icon\t\t/path"));
    CHECK(!parse("style-end\textra"));
}

static void test_color_parse(void)
{
    xfce_color color;

    CHECK(helper_color_parse("#ff800040", &color));
    CHECK_DOUBLE_NEAR(1.0, color.red, 1e-9);
    CHECK_DOUBLE_NEAR(128.0 / 255.0, color.green, 1e-9);
    CHECK_DOUBLE_NEAR(0.0, color.blue, 1e-9);
    CHECK_DOUBLE_NEAR(64.0 / 255.0, color.alpha, 1e-9);
    CHECK(!helper_color_parse("#ff8000", &color));
    CHECK(!helper_color_parse("ff800040", &color));
    CHECK(!helper_color_parse("#-f800040", &color));
}

static void test_icon_request_format(void)
{
    icon_request request = { .app_id = "org.app\tx", .icon_name = NULL, .size = 32 };
    char *line = helper_icon_request_format(5, &request);

    CHECK(strcmp(line, "icon\t5\t32\torg.app\\tx\t\n") == 0);
    g_free(line);
}

static void test_terminate_answer(void)
{
    CHECK(helper_terminate_answer("yes\n"));
    CHECK(helper_terminate_answer("yes"));
    CHECK(!helper_terminate_answer("no\n"));
    CHECK(!helper_terminate_answer("yesterday\n"));
    CHECK(!helper_terminate_answer(NULL));
}

static void feed(style_state *state, const char *line)
{
    CHECK(parse(line));
    style_state_apply(state, &message);
}

static void test_style_empty_until_end(void)
{
    style_state *state = style_state_create();
    xfce_style style = style_state_current(state);

    CHECK(!style.is_from_gtk);
    CHECK_INT_EQ(0, style.symbol_count);
    feed(state, "style-begin");
    feed(state, "symbol\tactive_text_color\t#111111111111");
    CHECK(!style_state_current(state).is_from_gtk);
    feed(state, "style-end");
    style = style_state_current(state);
    CHECK(style.is_from_gtk);
    CHECK(strcmp(style.symbols[0].value, "#111111111111") == 0);
    style_state_destroy(state);
}

static void test_style_colors_and_font(void)
{
    style_state *state = style_state_create();
    xfce_style style;

    feed(state, "style-begin");
    feed(state, "color\tselected_background\t#0000ffff");
    feed(state, "font\tSans 12");
    feed(state, "style-end");
    style = style_state_current(state);
    CHECK_DOUBLE_NEAR(1.0, style.selected_background.blue, 1e-9);
    CHECK_DOUBLE_NEAR(0.0, style.selected_background.red, 1e-9);
    CHECK(strcmp(style.font, "Sans 12") == 0);
    style_state_destroy(state);
}

static void feed_report(style_state *state, const char *font)
{
    char *line = g_strconcat("font\t", font, NULL);

    feed(state, "style-begin");
    feed(state, "symbol\tactive_text_color\t#111111111111");
    feed(state, line);
    g_free(line);
}

static void test_style_change_detected_only_on_difference(void)
{
    style_state *state = style_state_create();

    feed_report(state, "Sans 10");
    CHECK(style_state_pending_differs(state));
    feed(state, "style-end");
    feed_report(state, "Sans 10");
    CHECK(!style_state_pending_differs(state));
    feed(state, "style-end");
    feed_report(state, "Sans 11");
    CHECK(style_state_pending_differs(state));
    style_state_destroy(state);
}

static void test_change_flags(void)
{
    CHECK_INT_EQ(CHANGE_THEME, change_flags_for(CHANNEL_XFWM4, "/general/theme"));
    CHECK_INT_EQ(CHANGE_THEME, change_flags_for(CHANNEL_XFWM4, "/general/title_font"));
    CHECK_INT_EQ(CHANGE_SETTINGS, change_flags_for(CHANNEL_XFWM4, "/general/theme_extra"));
    CHECK_INT_EQ(CHANGE_SETTINGS, change_flags_for(CHANNEL_XFWM4, "/general/button_layout"));
    CHECK_INT_EQ(0, change_flags_for(CHANNEL_XFWM4, "/other"));
    CHECK_INT_EQ(CHANGE_SETTINGS | CHANGE_X11_SETTINGS,
                 change_flags_for(CHANNEL_XSETTINGS, "/Net/DoubleClickTime"));
    CHECK_INT_EQ(CHANGE_INPUT | CHANGE_X11_SETTINGS,
                 change_flags_for(CHANNEL_XSETTINGS, "/Gtk/CursorThemeSize"));
    CHECK_INT_EQ(CHANGE_X11_SETTINGS, change_flags_for(CHANNEL_XSETTINGS, "/Net/ThemeName"));
    CHECK_INT_EQ(CHANGE_SHORTCUTS, change_flags_for(CHANNEL_SHORTCUTS, "/xfwm4/custom/<Alt>F4"));
    CHECK_INT_EQ(CHANGE_INPUT, change_flags_for(CHANNEL_POINTERS, "/Mouse/RightHanded"));
}

int main(void)
{
    static const check_case cases[] = {
        { "parse_symbol", test_parse_symbol },
        { "parse_style_markers_and_font", test_parse_style_markers_and_font },
        { "parse_icon_reply", test_parse_icon_reply },
        { "parse_rejects_bad_lines", test_parse_rejects_bad_lines },
        { "color_parse", test_color_parse },
        { "icon_request_format", test_icon_request_format },
        { "terminate_answer", test_terminate_answer },
        { "style_empty_until_end", test_style_empty_until_end },
        { "style_change_detected_only_on_difference",
          test_style_change_detected_only_on_difference },
        { "style_colors_and_font", test_style_colors_and_font },
        { "change_flags", test_change_flags },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
