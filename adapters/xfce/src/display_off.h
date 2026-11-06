#ifndef XFCE_DISPLAY_OFF_H
#define XFCE_DISPLAY_OFF_H

#include <stdbool.h>
#include <stdint.h>

#include "setting_source.h"

uint32_t display_off_ms_from(const setting_source *settings, bool battery);

#endif
