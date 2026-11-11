#ifndef XFCE_CONFIG_VALUES_H
#define XFCE_CONFIG_VALUES_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/config.h"
#include "wm/rules.h"

bool config_bool(const char *text);
int config_int(const char *text);
wm_activate_action config_activate_action(const char *text);
wm_double_click_action config_double_click_action(const char *text);
wm_placement_mode config_placement_mode(const char *text);
wm_alignment config_alignment(const char *text);
wm_title_shadow config_title_shadow(const char *text);
wm_vblank_mode config_vblank_mode(const char *text);
wm_render_time config_render_time(const char *text);
uint32_t config_easy_click(const char *text);
wm_rule_anchor config_rule_anchor(const char *text);
wm_rule_match config_rule_match(const char *text);

#endif
