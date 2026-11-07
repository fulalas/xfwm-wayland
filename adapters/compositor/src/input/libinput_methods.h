#ifndef COMPOSITOR_LIBINPUT_METHODS_H
#define COMPOSITOR_LIBINPUT_METHODS_H

#include "compositor/services.h"

struct libinput_device;

void libinput_methods_apply_send_events(struct libinput_device *device,
                                        const pointer_prefs *prefs);
void libinput_methods_apply(struct libinput_device *device, const pointer_prefs *prefs);

#endif
