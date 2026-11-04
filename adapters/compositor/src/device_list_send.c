#include <wayland-server-core.h>

#include "device_list_send.h"
#include "xfce-input-device-list-private-v1-protocol.h"

enum {
    MATRIX_VALUES = 6,
    AREA_VALUES = 4,
};

typedef void (*send_three)(struct wl_resource *resource, uint32_t a, uint32_t b, uint32_t c);
typedef void (*send_two)(struct wl_resource *resource, uint32_t a, uint32_t b);

static const send_three three_word_events[SETTING_COUNT] = {
    [SETTING_SEND_EVENTS] = xfce_input_device_v1_send_send_events,
    [SETTING_ACCEL_PROFILE] = xfce_input_device_v1_send_accel_profile,
    [SETTING_SCROLL_METHOD] = xfce_input_device_v1_send_scroll_method,
    [SETTING_CLICK_METHOD] = xfce_input_device_v1_send_click_method,
    [SETTING_THREE_FINGER_DRAG] = xfce_input_device_v1_send_three_finger_drag,
};

static const send_two two_word_events[SETTING_COUNT] = {
    [SETTING_NATURAL_SCROLL] = xfce_input_device_v1_send_natural_scroll,
    [SETTING_SCROLL_BUTTON] = xfce_input_device_v1_send_scroll_button,
    [SETTING_SCROLL_BUTTON_LOCK] = xfce_input_device_v1_send_scroll_button_lock,
    [SETTING_CLICKFINGER_BUTTON_MAP] = xfce_input_device_v1_send_clickfinger_button_map,
    [SETTING_LEFT_HANDED] = xfce_input_device_v1_send_left_handed,
    [SETTING_MIDDLE_EMULATION] = xfce_input_device_v1_send_middle_emulation,
    [SETTING_TAP] = xfce_input_device_v1_send_tap,
    [SETTING_TAP_BUTTON_MAP] = xfce_input_device_v1_send_tap_button_map,
    [SETTING_TAP_DRAG] = xfce_input_device_v1_send_tap_drag,
    [SETTING_TAP_DRAG_LOCK] = xfce_input_device_v1_send_tap_drag_lock,
    [SETTING_DWT] = xfce_input_device_v1_send_dwt,
    [SETTING_DWT_TIMEOUT] = xfce_input_device_v1_send_dwt_timeout,
    [SETTING_DWTP] = xfce_input_device_v1_send_dwtp,
    [SETTING_DWTP_TIMEOUT] = xfce_input_device_v1_send_dwtp_timeout,
    [SETTING_ROTATION] = xfce_input_device_v1_send_rotation,
};

static void send_setting(struct wl_resource *resource, device_setting_id id,
                         const device_setting *setting);
static void send_numbers(struct wl_resource *resource, device_setting_id id,
                         const device_setting *setting);
static void send_calibration(struct wl_resource *resource, const double *numbers);
static void send_area(struct wl_resource *resource, const double *numbers);

void device_list_send_all(struct wl_resource *resource, const device_settings *settings)
{
    for (int id = 0; id < SETTING_COUNT; id++) {
        if (settings->items[id].available) {
            send_setting(resource, id, &settings->items[id]);
        }
    }
}

void device_list_send_changes(struct wl_resource *resource, const device_settings *before,
                              const device_settings *after)
{
    for (int id = 0; id < SETTING_COUNT; id++) {
        const device_setting *now = &after->items[id];
        if (now->available && !device_setting_equal(&before->items[id], now)) {
            send_setting(resource, id, now);
        }
    }
}

static void send_setting(struct wl_resource *resource, device_setting_id id,
                         const device_setting *setting)
{
    const uint32_t *w = setting->words;
    if (three_word_events[id] != NULL) {
        three_word_events[id](resource, w[0], w[1], w[2]);
    } else if (two_word_events[id] != NULL) {
        two_word_events[id](resource, w[0], w[1]);
    } else if (id == SETTING_TAP_FINGER_COUNT) {
        xfce_input_device_v1_send_tap_finger_count(resource, w[0]);
    } else {
        send_numbers(resource, id, setting);
    }
}

static void send_numbers(struct wl_resource *resource, device_setting_id id,
                         const device_setting *setting)
{
    const double *n = setting->numbers;
    if (id == SETTING_ACCEL_SPEED) {
        xfce_input_device_v1_send_accel_speed(resource, wl_fixed_from_double(n[0]),
                                              wl_fixed_from_double(n[1]));
    } else if (id == SETTING_CALIBRATION) {
        send_calibration(resource, n);
    } else if (id == SETTING_AREA) {
        send_area(resource, n);
    }
}

static void send_calibration(struct wl_resource *resource, const double *numbers)
{
    wl_fixed_t f[2 * MATRIX_VALUES];
    for (int i = 0; i < 2 * MATRIX_VALUES; i++) {
        f[i] = wl_fixed_from_double(numbers[i]);
    }
    xfce_input_device_v1_send_calibration(resource, f[0], f[1], f[2], f[3], f[4], f[5], f[6],
                                          f[7], f[8], f[9], f[10], f[11]);
}

static void send_area(struct wl_resource *resource, const double *numbers)
{
    wl_fixed_t f[2 * AREA_VALUES];
    for (int i = 0; i < 2 * AREA_VALUES; i++) {
        f[i] = wl_fixed_from_double(numbers[i]);
    }
    xfce_input_device_v1_send_area(resource, f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]);
}
