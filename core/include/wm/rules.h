#ifndef WM_RULES_H
#define WM_RULES_H

#include <stdbool.h>

typedef enum wm_rule_match {
    WM_MATCH_EXACT,
    WM_MATCH_GLOB,
} wm_rule_match;

typedef enum wm_rule_anchor {
    WM_ANCHOR_NONE,
    WM_ANCHOR_TOP_LEFT,
    WM_ANCHOR_TOP,
    WM_ANCHOR_TOP_RIGHT,
    WM_ANCHOR_LEFT,
    WM_ANCHOR_CENTER,
    WM_ANCHOR_RIGHT,
    WM_ANCHOR_BOTTOM_LEFT,
    WM_ANCHOR_BOTTOM,
    WM_ANCHOR_BOTTOM_RIGHT,
} wm_rule_anchor;

typedef struct wm_rule_switch {
    bool is_set;
    bool on;
} wm_rule_switch;

typedef struct wm_window_rule {
    const char *title;
    const char *app_id;
    wm_rule_match match;
    wm_rule_anchor anchor;
    int margin;
    const char *output;
    bool has_workspace;
    int workspace;
    wm_rule_switch above;
    wm_rule_switch skip_taskbar;
    wm_rule_switch no_focus;
} wm_window_rule;

#endif
