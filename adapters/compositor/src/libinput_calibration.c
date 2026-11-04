#include <libinput.h>

#include "input_transform.h"
#include "libinput_calibration.h"

static input_matrix base_calibration(struct libinput_device *device,
                                     const calibration_prefs *prefs);
static input_matrix from_floats(const float *values);

void libinput_calibration_apply(struct libinput_device *device, const absolute_prefs *prefs)
{
    if (!libinput_device_config_calibration_has_matrix(device)) {
        return;
    }
    input_matrix base = base_calibration(device, &prefs->calibration);
    input_matrix user = input_matrix_for_device(prefs->rotation_degrees, prefs->reflection);
    input_matrix combined = input_matrix_then(&base, &user);
    float matrix[INPUT_MATRIX_VALUES];
    for (int i = 0; i < INPUT_MATRIX_VALUES; i++) {
        matrix[i] = (float) combined.m[i];
    }
    libinput_device_config_calibration_set_matrix(device, matrix);
}

static input_matrix base_calibration(struct libinput_device *device,
                                     const calibration_prefs *prefs)
{
    if (prefs->is_set) {
        return from_floats(prefs->matrix);
    }
    float matrix[INPUT_MATRIX_VALUES];
    libinput_device_config_calibration_get_default_matrix(device, matrix);
    return from_floats(matrix);
}

static input_matrix from_floats(const float *values)
{
    input_matrix matrix;
    for (int i = 0; i < INPUT_MATRIX_VALUES; i++) {
        matrix.m[i] = values[i];
    }
    return matrix;
}
