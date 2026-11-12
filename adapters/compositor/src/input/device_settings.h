#ifndef COMPOSITOR_DEVICE_SETTINGS_H
#define COMPOSITOR_DEVICE_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

struct libinput_device;

typedef enum device_setting_id {
    SETTING_SEND_EVENTS,
    SETTING_ACCEL_PROFILE,
    SETTING_SCROLL_METHOD,
    SETTING_CLICK_METHOD,
    SETTING_THREE_FINGER_DRAG,
    SETTING_NATURAL_SCROLL,
    SETTING_SCROLL_BUTTON,
    SETTING_SCROLL_BUTTON_LOCK,
    SETTING_CLICKFINGER_BUTTON_MAP,
    SETTING_LEFT_HANDED,
    SETTING_MIDDLE_EMULATION,
    SETTING_TAP,
    SETTING_TAP_BUTTON_MAP,
    SETTING_TAP_DRAG,
    SETTING_TAP_DRAG_LOCK,
    SETTING_DWT,
    SETTING_DWT_TIMEOUT,
    SETTING_DWTP,
    SETTING_DWTP_TIMEOUT,
    SETTING_ROTATION,
    SETTING_TAP_FINGER_COUNT,
    SETTING_ACCEL_SPEED,
    SETTING_CALIBRATION,
    SETTING_AREA,
    SETTING_COUNT,
} device_setting_id;

enum {
    SETTING_WORDS = 3,
    SETTING_MATRIX_VALUES = 6,
    SETTING_AREA_VALUES = 4,
    SETTING_NUMBERS = 2 * SETTING_MATRIX_VALUES,
};

typedef struct device_setting {
    bool available;
    uint32_t words[SETTING_WORDS];
    double numbers[SETTING_NUMBERS];
} device_setting;

typedef struct device_settings {
    device_setting items[SETTING_COUNT];
} device_settings;

device_settings device_settings_read(struct libinput_device *device);
void device_settings_read_touchpad(struct libinput_device *device, device_settings *settings);
void device_settings_read_absolute(struct libinput_device *device, device_settings *settings);
device_setting device_setting_choice(uint32_t supported, uint32_t fallback, uint32_t current);
device_setting device_setting_pair(uint32_t fallback, uint32_t current);
bool device_setting_equal(const device_setting *a, const device_setting *b);
uint32_t device_capabilities(struct libinput_device *device);

#endif
