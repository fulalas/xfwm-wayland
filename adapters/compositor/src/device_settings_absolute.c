#include <libinput.h>

#include "device_settings.h"

enum {
    MATRIX_VALUES = 6,
    AREA_VALUES = 4,
};

static device_setting rotation(struct libinput_device *device);
static device_setting calibration(struct libinput_device *device);
static device_setting area(struct libinput_device *device);
static void copy_area(struct libinput_config_area_rectangle rectangle, double *target);

void device_settings_read_absolute(struct libinput_device *device, device_settings *settings)
{
    settings->items[SETTING_ROTATION] = rotation(device);
    settings->items[SETTING_CALIBRATION] = calibration(device);
    settings->items[SETTING_AREA] = area(device);
}

static device_setting rotation(struct libinput_device *device)
{
    if (!libinput_device_config_rotation_is_available(device)) {
        return (device_setting){0};
    }
    return device_setting_pair(libinput_device_config_rotation_get_default_angle(device),
                               libinput_device_config_rotation_get_angle(device));
}

static device_setting calibration(struct libinput_device *device)
{
    if (!libinput_device_config_calibration_has_matrix(device)) {
        return (device_setting){0};
    }
    float fallback[MATRIX_VALUES];
    float current[MATRIX_VALUES];
    libinput_device_config_calibration_get_default_matrix(device, fallback);
    libinput_device_config_calibration_get_matrix(device, current);
    device_setting setting = {.available = true};
    for (int i = 0; i < MATRIX_VALUES; i++) {
        setting.numbers[i] = fallback[i];
        setting.numbers[MATRIX_VALUES + i] = current[i];
    }
    return setting;
}

static device_setting area(struct libinput_device *device)
{
    if (!libinput_device_config_area_has_rectangle(device)) {
        return (device_setting){0};
    }
    device_setting setting = {.available = true};
    copy_area(libinput_device_config_area_get_default_rectangle(device), setting.numbers);
    copy_area(libinput_device_config_area_get_rectangle(device), setting.numbers + AREA_VALUES);
    return setting;
}

static void copy_area(struct libinput_config_area_rectangle rectangle, double *target)
{
    target[0] = rectangle.x1;
    target[1] = rectangle.y1;
    target[2] = rectangle.x2;
    target[3] = rectangle.y2;
}
