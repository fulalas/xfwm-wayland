#include <libinput.h>
#include <stddef.h>

#include "libinput_methods.h"

static void apply_profile(struct libinput_device *device, pointer_accel_profile profile);
static void apply_click(struct libinput_device *device, pointer_click_method method);
static void apply_scroll_method(struct libinput_device *device, pointer_scroll_method method);
static void apply_scroll_button(struct libinput_device *device, const pointer_prefs *prefs);
static enum libinput_config_accel_profile profile_of(pointer_accel_profile profile);
static enum libinput_config_click_method click_of(pointer_click_method method);
static enum libinput_config_scroll_method scroll_of(pointer_scroll_method method);

void libinput_methods_apply_send_events(struct libinput_device *device,
                                        const pointer_prefs *prefs)
{
    uint32_t mode = libinput_device_config_send_events_get_default_mode(device);
    if (prefs->enabled.is_set) {
        mode = prefs->enabled.value ? LIBINPUT_CONFIG_SEND_EVENTS_ENABLED
                                    : LIBINPUT_CONFIG_SEND_EVENTS_DISABLED;
    }
    bool supported = mode == LIBINPUT_CONFIG_SEND_EVENTS_ENABLED ||
                     (libinput_device_config_send_events_get_modes(device) & mode) != 0;
    if (supported) {
        libinput_device_config_send_events_set_mode(device, mode);
    }
}

void libinput_methods_apply(struct libinput_device *device, const pointer_prefs *prefs)
{
    apply_profile(device, prefs->accel_profile);
    apply_click(device, prefs->click_method);
    apply_scroll_method(device, prefs->scroll_method);
    apply_scroll_button(device, prefs);
}

static void apply_profile(struct libinput_device *device, pointer_accel_profile profile)
{
    if (!libinput_device_config_accel_is_available(device)) {
        return;
    }
    enum libinput_config_accel_profile wanted = profile_of(profile);
    if ((libinput_device_config_accel_get_profiles(device) & wanted) == 0) {
        wanted = libinput_device_config_accel_get_default_profile(device);
    }
    if (wanted != LIBINPUT_CONFIG_ACCEL_PROFILE_NONE) {
        libinput_device_config_accel_set_profile(device, wanted);
    }
}

static void apply_click(struct libinput_device *device, pointer_click_method method)
{
    uint32_t supported = libinput_device_config_click_get_methods(device);
    if (supported == LIBINPUT_CONFIG_CLICK_METHOD_NONE) {
        return;
    }
    enum libinput_config_click_method wanted = click_of(method);
    if (method == POINTER_CLICK_METHOD_DEFAULT ||
        (wanted != LIBINPUT_CONFIG_CLICK_METHOD_NONE && (supported & wanted) == 0)) {
        wanted = libinput_device_config_click_get_default_method(device);
    }
    libinput_device_config_click_set_method(device, wanted);
}

static void apply_scroll_method(struct libinput_device *device, pointer_scroll_method method)
{
    uint32_t supported = libinput_device_config_scroll_get_methods(device);
    if (supported == LIBINPUT_CONFIG_SCROLL_NO_SCROLL) {
        return;
    }
    enum libinput_config_scroll_method wanted = scroll_of(method);
    if (method == POINTER_SCROLL_METHOD_DEFAULT ||
        (wanted != LIBINPUT_CONFIG_SCROLL_NO_SCROLL && (supported & wanted) == 0)) {
        wanted = libinput_device_config_scroll_get_default_method(device);
    }
    libinput_device_config_scroll_set_method(device, wanted);
}

static void apply_scroll_button(struct libinput_device *device, const pointer_prefs *prefs)
{
    uint32_t supported = libinput_device_config_scroll_get_methods(device);
    if ((supported & LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN) == 0) {
        return;
    }
    uint32_t button = prefs->scroll_button.is_set
                          ? (uint32_t) prefs->scroll_button.value
                          : libinput_device_config_scroll_get_default_button(device);
    libinput_device_config_scroll_set_button(device, button);
    enum libinput_config_scroll_button_lock_state lock =
        libinput_device_config_scroll_get_default_button_lock(device);
    if (prefs->scroll_button_lock.is_set) {
        lock = prefs->scroll_button_lock.value ? LIBINPUT_CONFIG_SCROLL_BUTTON_LOCK_ENABLED
                                               : LIBINPUT_CONFIG_SCROLL_BUTTON_LOCK_DISABLED;
    }
    libinput_device_config_scroll_set_button_lock(device, lock);
}

static enum libinput_config_accel_profile profile_of(pointer_accel_profile profile)
{
    switch (profile) {
    case POINTER_ACCEL_PROFILE_ADAPTIVE:
        return LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
    case POINTER_ACCEL_PROFILE_FLAT:
        return LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;
    default:
        return LIBINPUT_CONFIG_ACCEL_PROFILE_NONE;
    }
}

static enum libinput_config_click_method click_of(pointer_click_method method)
{
    switch (method) {
    case POINTER_CLICK_METHOD_BUTTON_AREAS:
        return LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;
    case POINTER_CLICK_METHOD_CLICKFINGER:
        return LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER;
    default:
        return LIBINPUT_CONFIG_CLICK_METHOD_NONE;
    }
}

static enum libinput_config_scroll_method scroll_of(pointer_scroll_method method)
{
    switch (method) {
    case POINTER_SCROLL_METHOD_TWO_FINGER:
        return LIBINPUT_CONFIG_SCROLL_2FG;
    case POINTER_SCROLL_METHOD_EDGE:
        return LIBINPUT_CONFIG_SCROLL_EDGE;
    case POINTER_SCROLL_METHOD_ON_BUTTON_DOWN:
        return LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN;
    default:
        return LIBINPUT_CONFIG_SCROLL_NO_SCROLL;
    }
}
