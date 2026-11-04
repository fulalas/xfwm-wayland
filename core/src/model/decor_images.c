#include "decor_private.h"

enum {
    TOGGLED_IMAGE_SHIFT = WM_IMAGE_TOGGLED_ACTIVE - WM_IMAGE_ACTIVE,
};

static wm_button_image inactive_image(const decor_frame *frame, wm_button button);
static wm_button_image active_image(const decor_frame *frame, wm_button button);
static bool image_exists(const decor_frame *frame, wm_button button, wm_button_image image);
static wm_button_image resolved_image(const decor_frame *frame, wm_button button,
                                      wm_button_image image);
static bool is_toggled(const wm_decor_window *window, wm_button button);

wm_button_image decor_button_image(const decor_frame *frame, wm_button button)
{
    if (frame->state == WM_FRAME_INACTIVE) {
        return resolved_image(frame, button, inactive_image(frame, button));
    }
    return resolved_image(frame, button, active_image(frame, button));
}

static wm_button_image inactive_image(const decor_frame *frame, wm_button button)
{
    wm_button_status status = frame->window->buttons[button];
    if (status == WM_BUTTON_HOVERED && image_exists(frame, button, WM_IMAGE_INACTIVE_PRELIGHT)) {
        return WM_IMAGE_INACTIVE_PRELIGHT;
    }
    return WM_IMAGE_INACTIVE;
}

static wm_button_image active_image(const decor_frame *frame, wm_button button)
{
    wm_button_status status = frame->window->buttons[button];
    if (status == WM_BUTTON_PRESSED && image_exists(frame, button, WM_IMAGE_PRESSED)) {
        return WM_IMAGE_PRESSED;
    }
    if (status == WM_BUTTON_HOVERED && image_exists(frame, button, WM_IMAGE_PRELIGHT)) {
        return WM_IMAGE_PRELIGHT;
    }
    return WM_IMAGE_ACTIVE;
}

static bool image_exists(const decor_frame *frame, wm_button button, wm_button_image image)
{
    wm_button_image resolved = resolved_image(frame, button, image);
    return !wm_size_is_empty(frame->theme->button[button][resolved]);
}

static wm_button_image resolved_image(const decor_frame *frame, wm_button button,
                                      wm_button_image image)
{
    wm_button_image toggled = (wm_button_image) (image + TOGGLED_IMAGE_SHIFT);
    if (is_toggled(frame->window, button)
        && !wm_size_is_empty(frame->theme->button[button][toggled])) {
        return toggled;
    }
    return image;
}

static bool is_toggled(const wm_decor_window *window, wm_button button)
{
    switch (button) {
    case WM_BUTTON_SHADE:
        return window->shaded;
    case WM_BUTTON_STICK:
        return window->sticky;
    case WM_BUTTON_MAXIMIZE:
        return window->maximized_horizontally || window->maximized_vertically;
    default:
        return false;
    }
}
