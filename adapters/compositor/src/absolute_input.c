#include <libinput.h>
#include <stdlib.h>
#include <wlr/backend/libinput.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/util/transform.h>

#include "absolute_input.h"
#include "listener.h"
#include "output_identity.h"
#include "output_setting.h"

typedef struct mapped_device {
    absolute_input *owner;
    struct wlr_input_device *device;
    absolute_prefs prefs;
    struct wlr_output *output;
    bool calibrated_by_libinput;
    struct wl_list link;
    struct wl_listener destroy;
} mapped_device;

struct absolute_input {
    absolute_places places;
    struct wl_list devices;
    struct wl_listener layout_change;
};

static void handle_layout_change(struct wl_listener *listener, void *data);
static void handle_device_destroy(struct wl_listener *listener, void *data);
static mapped_device *find_device(const absolute_input *absolute,
                                  struct wlr_input_device *device);
static mapped_device *track_device(absolute_input *absolute, struct wlr_input_device *device);
static void free_device(mapped_device *mapped);
static void map_to_output(mapped_device *mapped);
static struct wlr_output *find_output(const absolute_input *absolute, const char *id);
static bool has_libinput_calibration(struct wlr_input_device *device);
static bool follows_output(const mapped_device *mapped);
static input_matrix user_matrix(const mapped_device *mapped);
static input_matrix output_matrix(const mapped_device *mapped);
static input_matrix undo_output_matrix(const mapped_device *mapped);

absolute_input *absolute_input_create(const absolute_places *places)
{
    absolute_input *absolute = calloc(1, sizeof(*absolute));
    if (absolute == NULL) {
        return NULL;
    }
    absolute->places = *places;
    wl_list_init(&absolute->devices);
    listener_connect(&absolute->layout_change, &places->layout->events.change,
                     handle_layout_change);
    return absolute;
}

void absolute_input_destroy(absolute_input *absolute)
{
    listener_disconnect(&absolute->layout_change);
    mapped_device *mapped;
    mapped_device *tmp;
    wl_list_for_each_safe(mapped, tmp, &absolute->devices, link) {
        free_device(mapped);
    }
    free(absolute);
}

void absolute_input_configure(absolute_input *absolute, struct wlr_input_device *device,
                              const absolute_prefs *prefs)
{
    if (device->type != WLR_INPUT_DEVICE_TOUCH && device->type != WLR_INPUT_DEVICE_TABLET) {
        return;
    }
    mapped_device *mapped = find_device(absolute, device);
    if (mapped == NULL) {
        mapped = track_device(absolute, device);
    }
    if (mapped != NULL) {
        mapped->prefs = *prefs;
        mapped->calibrated_by_libinput = has_libinput_calibration(device);
        map_to_output(mapped);
    }
}

input_point absolute_input_position(const absolute_input *absolute,
                                    struct wlr_input_device *device, input_point where)
{
    const mapped_device *mapped = find_device(absolute, device);
    if (mapped == NULL || mapped->calibrated_by_libinput) {
        return where;
    }
    input_matrix undo = undo_output_matrix(mapped);
    input_matrix user = user_matrix(mapped);
    input_matrix motion = input_matrix_then(&undo, &user);
    if (follows_output(mapped)) {
        input_matrix redo = output_matrix(mapped);
        motion = input_matrix_then(&motion, &redo);
    }
    return input_matrix_apply(&motion, where);
}

input_point absolute_input_tilt(const absolute_input *absolute, struct wlr_input_device *device,
                                input_point tilt)
{
    const mapped_device *mapped = find_device(absolute, device);
    if (mapped == NULL) {
        return tilt;
    }
    input_matrix user = user_matrix(mapped);
    input_point turned = input_matrix_turn(&user, tilt);
    if (!follows_output(mapped)) {
        return turned;
    }
    input_matrix output = output_matrix(mapped);
    return input_matrix_turn(&output, turned);
}

static void handle_layout_change(struct wl_listener *listener, void *data)
{
    (void) data;
    absolute_input *absolute = wl_container_of(listener, absolute, layout_change);
    mapped_device *mapped;
    wl_list_for_each(mapped, &absolute->devices, link) {
        map_to_output(mapped);
    }
}

static void handle_device_destroy(struct wl_listener *listener, void *data)
{
    (void) data;
    mapped_device *mapped = wl_container_of(listener, mapped, destroy);
    free_device(mapped);
}

static mapped_device *find_device(const absolute_input *absolute,
                                  struct wlr_input_device *device)
{
    mapped_device *mapped;
    wl_list_for_each(mapped, &absolute->devices, link) {
        if (mapped->device == device) {
            return mapped;
        }
    }
    return NULL;
}

static mapped_device *track_device(absolute_input *absolute, struct wlr_input_device *device)
{
    mapped_device *mapped = calloc(1, sizeof(*mapped));
    if (mapped == NULL) {
        return NULL;
    }
    mapped->owner = absolute;
    mapped->device = device;
    listener_connect(&mapped->destroy, &device->events.destroy, handle_device_destroy);
    wl_list_insert(&absolute->devices, &mapped->link);
    return mapped;
}

static void free_device(mapped_device *mapped)
{
    listener_disconnect(&mapped->destroy);
    wl_list_remove(&mapped->link);
    free(mapped);
}

static void map_to_output(mapped_device *mapped)
{
    absolute_input *absolute = mapped->owner;
    mapped->output = find_output(absolute, mapped->prefs.output_id);
    wlr_cursor_map_input_to_output(absolute->places.cursor, mapped->device, mapped->output);
}

static struct wlr_output *find_output(const absolute_input *absolute, const char *id)
{
    struct wlr_output_layout_output *placed;
    wl_list_for_each(placed, &absolute->places.layout->outputs, link) {
        output *known = output_from_wlr(placed->output);
        if (known == NULL || !placed->output->enabled) {
            continue;
        }
        display_identity identity = output_display_identity(known);
        if (output_identity_matches(&identity, id)) {
            return placed->output;
        }
    }
    return NULL;
}

static bool has_libinput_calibration(struct wlr_input_device *device)
{
    if (!wlr_input_device_is_libinput(device)) {
        return false;
    }
    struct libinput_device *handle = wlr_libinput_get_device_handle(device);
    return handle != NULL && libinput_device_config_calibration_has_matrix(handle);
}

static bool follows_output(const mapped_device *mapped)
{
    return mapped->device->type == WLR_INPUT_DEVICE_TOUCH || mapped->calibrated_by_libinput;
}

static input_matrix user_matrix(const mapped_device *mapped)
{
    return input_matrix_for_device(mapped->prefs.rotation_degrees, mapped->prefs.reflection);
}

static input_matrix output_matrix(const mapped_device *mapped)
{
    if (mapped->output == NULL) {
        return input_matrix_identity();
    }
    return input_matrix_for_output(mapped->output->transform);
}

static input_matrix undo_output_matrix(const mapped_device *mapped)
{
    if (mapped->output == NULL) {
        return input_matrix_identity();
    }
    return input_matrix_for_output(wlr_output_transform_invert(mapped->output->transform));
}
