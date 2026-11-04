#include <libinput.h>
#include <wlr/backend/libinput.h>

#include "libinput_calibration.h"
#include "libinput_methods.h"
#include "seat_private.h"

static void apply_tap(struct libinput_device *device, const pointer_prefs *prefs);
static void apply_scroll(struct libinput_device *device, const pointer_prefs *prefs);
static void apply_handedness(struct libinput_device *device, const pointer_prefs *prefs);
static void apply_accel(struct libinput_device *device, const pointer_prefs *prefs);
static void apply_typing(struct libinput_device *device, const pointer_prefs *prefs);
static void apply_middle(struct libinput_device *device, const pointer_prefs *prefs);
static bool chosen(optional_bool choice, int device_default);

void libinput_config_apply(struct wlr_input_device *device, const pointer_prefs *prefs)
{
    if (!wlr_input_device_is_libinput(device)) {
        return;
    }
    struct libinput_device *handle = wlr_libinput_get_device_handle(device);
    if (handle == NULL) {
        return;
    }
    libinput_methods_apply_send_events(handle, prefs);
    apply_tap(handle, prefs);
    apply_scroll(handle, prefs);
    apply_handedness(handle, prefs);
    apply_accel(handle, prefs);
    apply_typing(handle, prefs);
    apply_middle(handle, prefs);
    libinput_methods_apply(handle, prefs);
    libinput_calibration_apply(handle, &prefs->absolute);
}

static void apply_tap(struct libinput_device *device, const pointer_prefs *prefs)
{
    if (libinput_device_config_tap_get_finger_count(device) <= 0) {
        return;
    }
    bool enabled = prefs->tap.is_set ? prefs->tap.value : true;
    libinput_device_config_tap_set_enabled(device, enabled ? LIBINPUT_CONFIG_TAP_ENABLED
                                                           : LIBINPUT_CONFIG_TAP_DISABLED);
}

static void apply_scroll(struct libinput_device *device, const pointer_prefs *prefs)
{
    if (!libinput_device_config_scroll_has_natural_scroll(device)) {
        return;
    }
    int fallback = libinput_device_config_scroll_get_default_natural_scroll_enabled(device);
    libinput_device_config_scroll_set_natural_scroll_enabled(
        device, chosen(prefs->natural_scroll, fallback));
}

static void apply_handedness(struct libinput_device *device, const pointer_prefs *prefs)
{
    if (!libinput_device_config_left_handed_is_available(device)) {
        return;
    }
    int fallback = libinput_device_config_left_handed_get_default(device);
    libinput_device_config_left_handed_set(device, chosen(prefs->left_handed, fallback));
}

static void apply_accel(struct libinput_device *device, const pointer_prefs *prefs)
{
    if (!libinput_device_config_accel_is_available(device)) {
        return;
    }
    double speed = prefs->accel_speed.is_set
                       ? prefs->accel_speed.value
                       : libinput_device_config_accel_get_default_speed(device);
    speed = speed < -1.0 ? -1.0 : (speed > 1.0 ? 1.0 : speed);
    libinput_device_config_accel_set_speed(device, speed);
}

static void apply_typing(struct libinput_device *device, const pointer_prefs *prefs)
{
    if (!libinput_device_config_dwt_is_available(device)) {
        return;
    }
    int fallback = libinput_device_config_dwt_get_default_enabled(device);
    libinput_device_config_dwt_set_enabled(device,
                                           chosen(prefs->disable_while_typing, fallback)
                                               ? LIBINPUT_CONFIG_DWT_ENABLED
                                               : LIBINPUT_CONFIG_DWT_DISABLED);
}

static void apply_middle(struct libinput_device *device, const pointer_prefs *prefs)
{
    if (!libinput_device_config_middle_emulation_is_available(device)) {
        return;
    }
    int fallback = libinput_device_config_middle_emulation_get_default_enabled(device);
    libinput_device_config_middle_emulation_set_enabled(
        device, chosen(prefs->middle_emulation, fallback)
                    ? LIBINPUT_CONFIG_MIDDLE_EMULATION_ENABLED
                    : LIBINPUT_CONFIG_MIDDLE_EMULATION_DISABLED);
}

static bool chosen(optional_bool choice, int device_default)
{
    return choice.is_set ? choice.value : device_default != 0;
}
