#include <libinput.h>
#include <stddef.h>

#include "device_settings.h"

static device_setting send_events(struct libinput_device *device);
static device_setting accel_speed(struct libinput_device *device);
static device_setting accel_profile(struct libinput_device *device);
static device_setting natural_scroll(struct libinput_device *device);
static void read_scrolling(struct libinput_device *device, device_settings *settings);
static void read_clicking(struct libinput_device *device, device_settings *settings);
static device_setting left_handed(struct libinput_device *device);
static device_setting middle_emulation(struct libinput_device *device);

static const enum libinput_device_capability capabilities[] = {
    LIBINPUT_DEVICE_CAP_KEYBOARD,   LIBINPUT_DEVICE_CAP_POINTER, LIBINPUT_DEVICE_CAP_TOUCH,
    LIBINPUT_DEVICE_CAP_TABLET_TOOL, LIBINPUT_DEVICE_CAP_TABLET_PAD,
    LIBINPUT_DEVICE_CAP_GESTURE,    LIBINPUT_DEVICE_CAP_SWITCH,
};

device_settings device_settings_read(struct libinput_device *device)
{
    device_settings settings = {0};
    settings.items[SETTING_SEND_EVENTS] = send_events(device);
    settings.items[SETTING_ACCEL_SPEED] = accel_speed(device);
    settings.items[SETTING_ACCEL_PROFILE] = accel_profile(device);
    settings.items[SETTING_NATURAL_SCROLL] = natural_scroll(device);
    settings.items[SETTING_LEFT_HANDED] = left_handed(device);
    settings.items[SETTING_MIDDLE_EMULATION] = middle_emulation(device);
    read_scrolling(device, &settings);
    read_clicking(device, &settings);
    device_settings_read_touchpad(device, &settings);
    device_settings_read_absolute(device, &settings);
    return settings;
}

uint32_t device_capabilities(struct libinput_device *device)
{
    uint32_t bits = 0;
    for (size_t i = 0; i < sizeof(capabilities) / sizeof(capabilities[0]); i++) {
        if (libinput_device_has_capability(device, capabilities[i])) {
            bits |= 1u << capabilities[i];
        }
    }
    return bits;
}

static device_setting send_events(struct libinput_device *device)
{
    uint32_t modes = libinput_device_config_send_events_get_modes(device);
    uint32_t fallback = libinput_device_config_send_events_get_default_mode(device);
    return device_setting_choice(modes, fallback,
                                 libinput_device_config_send_events_get_mode(device));
}

static device_setting accel_speed(struct libinput_device *device)
{
    if (!libinput_device_config_accel_is_available(device)) {
        return (device_setting){0};
    }
    return (device_setting){
        .available = true,
        .numbers = {libinput_device_config_accel_get_default_speed(device),
                    libinput_device_config_accel_get_speed(device)},
    };
}

static device_setting accel_profile(struct libinput_device *device)
{
    uint32_t profiles = libinput_device_config_accel_get_profiles(device);
    if (profiles == LIBINPUT_CONFIG_ACCEL_PROFILE_NONE) {
        return (device_setting){0};
    }
    uint32_t fallback = libinput_device_config_accel_get_default_profile(device);
    return device_setting_choice(profiles, fallback,
                                 libinput_device_config_accel_get_profile(device));
}

static device_setting natural_scroll(struct libinput_device *device)
{
    if (!libinput_device_config_scroll_has_natural_scroll(device)) {
        return (device_setting){0};
    }
    uint32_t fallback = libinput_device_config_scroll_get_default_natural_scroll_enabled(device);
    return device_setting_pair(fallback,
                               libinput_device_config_scroll_get_natural_scroll_enabled(device));
}

static void read_scrolling(struct libinput_device *device, device_settings *settings)
{
    uint32_t methods = libinput_device_config_scroll_get_methods(device);
    if (methods == LIBINPUT_CONFIG_SCROLL_NO_SCROLL) {
        return;
    }
    uint32_t fallback = libinput_device_config_scroll_get_default_method(device);
    settings->items[SETTING_SCROLL_METHOD] = device_setting_choice(
        methods, fallback, libinput_device_config_scroll_get_method(device));
    if ((methods & LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN) == 0) {
        return;
    }
    settings->items[SETTING_SCROLL_BUTTON] =
        device_setting_pair(libinput_device_config_scroll_get_default_button(device),
                            libinput_device_config_scroll_get_button(device));
    settings->items[SETTING_SCROLL_BUTTON_LOCK] =
        device_setting_pair(libinput_device_config_scroll_get_default_button_lock(device),
                            libinput_device_config_scroll_get_button_lock(device));
}

static void read_clicking(struct libinput_device *device, device_settings *settings)
{
    uint32_t methods = libinput_device_config_click_get_methods(device);
    if (methods == LIBINPUT_CONFIG_CLICK_METHOD_NONE) {
        return;
    }
    uint32_t fallback = libinput_device_config_click_get_default_method(device);
    settings->items[SETTING_CLICK_METHOD] = device_setting_choice(
        methods, fallback, libinput_device_config_click_get_method(device));
    if ((methods & LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER) == 0) {
        return;
    }
    uint32_t map = libinput_device_config_click_get_default_clickfinger_button_map(device);
    settings->items[SETTING_CLICKFINGER_BUTTON_MAP] = device_setting_pair(
        map, libinput_device_config_click_get_clickfinger_button_map(device));
}

static device_setting left_handed(struct libinput_device *device)
{
    if (!libinput_device_config_left_handed_is_available(device)) {
        return (device_setting){0};
    }
    return device_setting_pair(libinput_device_config_left_handed_get_default(device),
                               libinput_device_config_left_handed_get(device));
}

static device_setting middle_emulation(struct libinput_device *device)
{
    if (!libinput_device_config_middle_emulation_is_available(device)) {
        return (device_setting){0};
    }
    uint32_t fallback = libinput_device_config_middle_emulation_get_default_enabled(device);
    return device_setting_pair(fallback,
                               libinput_device_config_middle_emulation_get_enabled(device));
}
