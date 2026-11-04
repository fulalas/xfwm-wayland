#ifndef COMPOSITOR_OUTPUT_SETTING_H
#define COMPOSITOR_OUTPUT_SETTING_H

#include <stdbool.h>

#include "compositor/displays.h"
#include "output.h"
#include "output_display.h"

output_display *output_display_of(output *output);
display_identity output_display_identity(const output *output);
display_setting output_current_setting(const output *output);
display_setting output_saved_setting(const output *output);
bool output_apply_at(output *output, const display_setting *setting);
bool output_apply_auto(output *output);
void output_turn_off(output *output);
void output_remember(output *output);

#endif
