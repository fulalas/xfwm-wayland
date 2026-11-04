#include <libinput.h>

#include "device_settings.h"

enum {
    THREE_FINGERS = 3,
};

static void read_tapping(struct libinput_device *device, device_settings *settings);
static void read_typing(struct libinput_device *device, device_settings *settings);
static void read_trackpointing(struct libinput_device *device, device_settings *settings);
static device_setting three_finger_drag(struct libinput_device *device);
static device_setting count(uint32_t value);

void device_settings_read_touchpad(struct libinput_device *device, device_settings *settings)
{
    read_tapping(device, settings);
    read_typing(device, settings);
    read_trackpointing(device, settings);
    settings->items[SETTING_THREE_FINGER_DRAG] = three_finger_drag(device);
}

static void read_tapping(struct libinput_device *device, device_settings *settings)
{
    int fingers = libinput_device_config_tap_get_finger_count(device);
    if (fingers <= 0) {
        return;
    }
    settings->items[SETTING_TAP_FINGER_COUNT] = count((uint32_t) fingers);
    settings->items[SETTING_TAP] =
        device_setting_pair(libinput_device_config_tap_get_default_enabled(device),
                            libinput_device_config_tap_get_enabled(device));
    settings->items[SETTING_TAP_BUTTON_MAP] =
        device_setting_pair(libinput_device_config_tap_get_default_button_map(device),
                            libinput_device_config_tap_get_button_map(device));
    settings->items[SETTING_TAP_DRAG] =
        device_setting_pair(libinput_device_config_tap_get_default_drag_enabled(device),
                            libinput_device_config_tap_get_drag_enabled(device));
    settings->items[SETTING_TAP_DRAG_LOCK] =
        device_setting_pair(libinput_device_config_tap_get_default_drag_lock_enabled(device),
                            libinput_device_config_tap_get_drag_lock_enabled(device));
}

static void read_typing(struct libinput_device *device, device_settings *settings)
{
    if (!libinput_device_config_dwt_is_available(device)) {
        return;
    }
    settings->items[SETTING_DWT] =
        device_setting_pair(libinput_device_config_dwt_get_default_enabled(device),
                            libinput_device_config_dwt_get_enabled(device));
    uint32_t timeout = libinput_device_config_dwt_get_default_timeout(device);
    if (timeout > 0) {
        settings->items[SETTING_DWT_TIMEOUT] =
            device_setting_pair(timeout, libinput_device_config_dwt_get_timeout(device));
    }
}

static void read_trackpointing(struct libinput_device *device, device_settings *settings)
{
    if (!libinput_device_config_dwtp_is_available(device)) {
        return;
    }
    settings->items[SETTING_DWTP] =
        device_setting_pair(libinput_device_config_dwtp_get_default_enabled(device),
                            libinput_device_config_dwtp_get_enabled(device));
    uint32_t timeout = libinput_device_config_dwtp_get_default_timeout(device);
    if (timeout > 0) {
        settings->items[SETTING_DWTP_TIMEOUT] =
            device_setting_pair(timeout, libinput_device_config_dwtp_get_timeout(device));
    }
}

static device_setting three_finger_drag(struct libinput_device *device)
{
    int fingers = libinput_device_config_3fg_drag_get_finger_count(device);
    if (fingers < THREE_FINGERS) {
        return (device_setting){0};
    }
    return device_setting_choice((uint32_t) fingers,
                                 libinput_device_config_3fg_drag_get_default_enabled(device),
                                 libinput_device_config_3fg_drag_get_enabled(device));
}

static device_setting count(uint32_t value)
{
    return (device_setting){.available = true, .words = {value}};
}
