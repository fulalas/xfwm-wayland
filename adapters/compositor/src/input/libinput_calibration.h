#ifndef COMPOSITOR_LIBINPUT_CALIBRATION_H
#define COMPOSITOR_LIBINPUT_CALIBRATION_H

#include "compositor/services.h"

struct libinput_device;

void libinput_calibration_apply(struct libinput_device *device, const absolute_prefs *prefs);

#endif
