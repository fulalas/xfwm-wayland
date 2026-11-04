#include <libinput.h>
#include <wlr/backend/libinput.h>

#include "seat_private.h"

static void apply_tap(struct libinput_device *device, const pointer_prefs *prefs);
static void apply_scroll(struct libinput_device *device, const pointer_prefs *prefs);
static void apply_handedness(struct libinput_device *device, const pointer_prefs *prefs);
static void apply_accel(struct libinput_device *device, const pointer_prefs *prefs);
static void apply_typing(struct libinput_device *device, const pointer_prefs *prefs);
static void apply_middle(struct libinput_device *device, const pointer_prefs *prefs);
static bool same_choice(optional_bool a, optional_bool b);
static bool same_speed(optional_double a, optional_double b);

void libinput_config_apply(struct wlr_input_device *device, const pointer_prefs *prefs)
{
    if (!wlr_input_device_is_libinput(device)) {
        return;
    }
    struct libinput_device *handle = wlr_libinput_get_device_handle(device);
    if (handle == NULL) {
        return;
    }
    apply_tap(handle, prefs);
    apply_scroll(handle, prefs);
    apply_handedness(handle, prefs);
    apply_accel(handle, prefs);
    apply_typing(handle, prefs);
    apply_middle(handle, prefs);
}

bool pointer_prefs_equal(const pointer_prefs *a, const pointer_prefs *b)
{
    return same_choice(a->tap, b->tap) && same_choice(a->natural_scroll, b->natural_scroll) &&
           same_choice(a->left_handed, b->left_handed) &&
           same_choice(a->disable_while_typing, b->disable_while_typing) &&
           same_choice(a->middle_emulation, b->middle_emulation) &&
           same_speed(a->accel_speed, b->accel_speed);
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
    if (prefs->natural_scroll.is_set && libinput_device_config_scroll_has_natural_scroll(device)) {
        libinput_device_config_scroll_set_natural_scroll_enabled(device,
                                                                 prefs->natural_scroll.value);
    }
}

static void apply_handedness(struct libinput_device *device, const pointer_prefs *prefs)
{
    if (prefs->left_handed.is_set && libinput_device_config_left_handed_is_available(device)) {
        libinput_device_config_left_handed_set(device, prefs->left_handed.value);
    }
}

static void apply_accel(struct libinput_device *device, const pointer_prefs *prefs)
{
    if (!prefs->accel_speed.is_set || !libinput_device_config_accel_is_available(device)) {
        return;
    }
    double speed = prefs->accel_speed.value;
    speed = speed < -1.0 ? -1.0 : (speed > 1.0 ? 1.0 : speed);
    libinput_device_config_accel_set_speed(device, speed);
}

static void apply_typing(struct libinput_device *device, const pointer_prefs *prefs)
{
    if (!prefs->disable_while_typing.is_set || !libinput_device_config_dwt_is_available(device)) {
        return;
    }
    libinput_device_config_dwt_set_enabled(device, prefs->disable_while_typing.value
                                                       ? LIBINPUT_CONFIG_DWT_ENABLED
                                                       : LIBINPUT_CONFIG_DWT_DISABLED);
}

static void apply_middle(struct libinput_device *device, const pointer_prefs *prefs)
{
    if (!prefs->middle_emulation.is_set ||
        !libinput_device_config_middle_emulation_is_available(device)) {
        return;
    }
    libinput_device_config_middle_emulation_set_enabled(
        device, prefs->middle_emulation.value ? LIBINPUT_CONFIG_MIDDLE_EMULATION_ENABLED
                                              : LIBINPUT_CONFIG_MIDDLE_EMULATION_DISABLED);
}

static bool same_choice(optional_bool a, optional_bool b)
{
    return a.is_set == b.is_set && (!a.is_set || a.value == b.value);
}

static bool same_speed(optional_double a, optional_double b)
{
    return a.is_set == b.is_set && (!a.is_set || a.value == b.value);
}
