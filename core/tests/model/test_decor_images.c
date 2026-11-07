#include "check.h"
#include "decor_fixture.h"

static const wm_size image_size = {FIXTURE_BUTTON, FIXTURE_BUTTON};

static decor_fixture fixture_with_status(wm_button button, wm_button_status status)
{
    decor_fixture fixture = fixture_create();
    fixture.window.buttons[button] = status;
    return fixture;
}

static void add_image(decor_fixture *fixture, wm_button button, wm_button_image image)
{
    fixture->theme.button[button][image] = image_size;
}

static void test_normal_button_uses_state_image(void)
{
    decor_fixture fixture = fixture_create();
    CHECK_INT_EQ(WM_IMAGE_ACTIVE, fixture_button_image(&fixture, WM_BUTTON_CLOSE));
    fixture.window.state = WM_FRAME_INACTIVE;
    CHECK_INT_EQ(WM_IMAGE_INACTIVE, fixture_button_image(&fixture, WM_BUTTON_CLOSE));
}

static void test_hover_uses_prelight_when_available(void)
{
    decor_fixture fixture = fixture_with_status(WM_BUTTON_CLOSE, WM_BUTTON_HOVERED);
    CHECK_INT_EQ(WM_IMAGE_ACTIVE, fixture_button_image(&fixture, WM_BUTTON_CLOSE));
    add_image(&fixture, WM_BUTTON_CLOSE, WM_IMAGE_PRELIGHT);
    CHECK_INT_EQ(WM_IMAGE_PRELIGHT, fixture_button_image(&fixture, WM_BUTTON_CLOSE));
}

static void test_press_uses_pressed_when_available(void)
{
    decor_fixture fixture = fixture_with_status(WM_BUTTON_CLOSE, WM_BUTTON_PRESSED);
    add_image(&fixture, WM_BUTTON_CLOSE, WM_IMAGE_PRELIGHT);
    CHECK_INT_EQ(WM_IMAGE_ACTIVE, fixture_button_image(&fixture, WM_BUTTON_CLOSE));
    add_image(&fixture, WM_BUTTON_CLOSE, WM_IMAGE_PRESSED);
    CHECK_INT_EQ(WM_IMAGE_PRESSED, fixture_button_image(&fixture, WM_BUTTON_CLOSE));
}

static void test_inactive_hover_uses_inactive_prelight(void)
{
    decor_fixture fixture = fixture_with_status(WM_BUTTON_CLOSE, WM_BUTTON_HOVERED);
    fixture.window.state = WM_FRAME_INACTIVE;
    add_image(&fixture, WM_BUTTON_CLOSE, WM_IMAGE_PRELIGHT);
    CHECK_INT_EQ(WM_IMAGE_INACTIVE, fixture_button_image(&fixture, WM_BUTTON_CLOSE));
    add_image(&fixture, WM_BUTTON_CLOSE, WM_IMAGE_INACTIVE_PRELIGHT);
    CHECK_INT_EQ(WM_IMAGE_INACTIVE_PRELIGHT, fixture_button_image(&fixture, WM_BUTTON_CLOSE));
}

static void test_inactive_press_is_never_pressed(void)
{
    decor_fixture fixture = fixture_with_status(WM_BUTTON_CLOSE, WM_BUTTON_PRESSED);
    fixture.window.state = WM_FRAME_INACTIVE;
    add_image(&fixture, WM_BUTTON_CLOSE, WM_IMAGE_PRESSED);
    CHECK_INT_EQ(WM_IMAGE_INACTIVE, fixture_button_image(&fixture, WM_BUTTON_CLOSE));
}

static void test_maximized_window_uses_toggled_maximize(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.maximized_horizontally = true;
    CHECK_INT_EQ(WM_IMAGE_ACTIVE, fixture_button_image(&fixture, WM_BUTTON_MAXIMIZE));
    add_image(&fixture, WM_BUTTON_MAXIMIZE, WM_IMAGE_TOGGLED_ACTIVE);
    CHECK_INT_EQ(WM_IMAGE_TOGGLED_ACTIVE, fixture_button_image(&fixture, WM_BUTTON_MAXIMIZE));
}

static void test_shaded_window_uses_toggled_shade(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.shaded = true;
    fixture.window.state = WM_FRAME_INACTIVE;
    add_image(&fixture, WM_BUTTON_SHADE, WM_IMAGE_TOGGLED_INACTIVE);
    CHECK_INT_EQ(WM_IMAGE_TOGGLED_INACTIVE, fixture_button_image(&fixture, WM_BUTTON_SHADE));
}

static void test_sticky_window_uses_toggled_stick(void)
{
    decor_fixture fixture = fixture_create();
    fixture_set_layout(&fixture, "T|C");
    fixture.window.sticky = true;
    add_image(&fixture, WM_BUTTON_STICK, WM_IMAGE_TOGGLED_ACTIVE);
    CHECK_INT_EQ(WM_IMAGE_TOGGLED_ACTIVE, fixture_button_image(&fixture, WM_BUTTON_STICK));
}

static void test_toggle_does_not_apply_to_other_buttons(void)
{
    decor_fixture fixture = fixture_create();
    fixture.window.shaded = true;
    add_image(&fixture, WM_BUTTON_CLOSE, WM_IMAGE_TOGGLED_ACTIVE);
    CHECK_INT_EQ(WM_IMAGE_ACTIVE, fixture_button_image(&fixture, WM_BUTTON_CLOSE));
}

static void test_toggled_hover_uses_toggled_prelight(void)
{
    decor_fixture fixture = fixture_with_status(WM_BUTTON_SHADE, WM_BUTTON_HOVERED);
    fixture.window.shaded = true;
    add_image(&fixture, WM_BUTTON_SHADE, WM_IMAGE_TOGGLED_PRELIGHT);
    CHECK_INT_EQ(WM_IMAGE_TOGGLED_PRELIGHT, fixture_button_image(&fixture, WM_BUTTON_SHADE));
}

static void test_toggled_hover_falls_back_to_plain_prelight(void)
{
    decor_fixture fixture = fixture_with_status(WM_BUTTON_SHADE, WM_BUTTON_HOVERED);
    fixture.window.shaded = true;
    add_image(&fixture, WM_BUTTON_SHADE, WM_IMAGE_TOGGLED_ACTIVE);
    add_image(&fixture, WM_BUTTON_SHADE, WM_IMAGE_PRELIGHT);
    CHECK_INT_EQ(WM_IMAGE_PRELIGHT, fixture_button_image(&fixture, WM_BUTTON_SHADE));
}

static void test_toggled_press_without_toggled_pressed_uses_pressed(void)
{
    decor_fixture fixture = fixture_with_status(WM_BUTTON_MAXIMIZE, WM_BUTTON_PRESSED);
    fixture.window.maximized_vertically = true;
    add_image(&fixture, WM_BUTTON_MAXIMIZE, WM_IMAGE_TOGGLED_ACTIVE);
    add_image(&fixture, WM_BUTTON_MAXIMIZE, WM_IMAGE_PRESSED);
    CHECK_INT_EQ(WM_IMAGE_PRESSED, fixture_button_image(&fixture, WM_BUTTON_MAXIMIZE));
}

static void test_missing_image_paints_nothing_but_keeps_hit_area(void)
{
    decor_fixture fixture = fixture_create();
    fixture.theme.button[WM_BUTTON_CLOSE][WM_IMAGE_ACTIVE] = (wm_size) {0, 0};
    wm_decor_layout layout = fixture_layout(&fixture);
    CHECK(fixture_find_op(&layout, WM_PIXMAP_BUTTON, WM_BUTTON_CLOSE) == NULL);
}

static const check_case CASES[] = {
    {"normal_button_uses_state_image", test_normal_button_uses_state_image},
    {"hover_uses_prelight_when_available", test_hover_uses_prelight_when_available},
    {"press_uses_pressed_when_available", test_press_uses_pressed_when_available},
    {"inactive_hover_uses_inactive_prelight", test_inactive_hover_uses_inactive_prelight},
    {"inactive_press_is_never_pressed", test_inactive_press_is_never_pressed},
    {"maximized_window_uses_toggled_maximize", test_maximized_window_uses_toggled_maximize},
    {"shaded_window_uses_toggled_shade", test_shaded_window_uses_toggled_shade},
    {"sticky_window_uses_toggled_stick", test_sticky_window_uses_toggled_stick},
    {"toggle_does_not_apply_to_other_buttons", test_toggle_does_not_apply_to_other_buttons},
    {"toggled_hover_uses_toggled_prelight", test_toggled_hover_uses_toggled_prelight},
    {"toggled_hover_falls_back_to_plain_prelight",
     test_toggled_hover_falls_back_to_plain_prelight},
    {"toggled_press_without_toggled_pressed_uses_pressed",
     test_toggled_press_without_toggled_pressed_uses_pressed},
    {"missing_image_paints_nothing", test_missing_image_paints_nothing_but_keeps_hit_area},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}
