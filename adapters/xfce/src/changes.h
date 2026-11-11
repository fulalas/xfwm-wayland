#ifndef XFCE_CHANGES_H
#define XFCE_CHANGES_H

typedef enum channel_id {
    CHANNEL_XFWM4,
    CHANNEL_XSETTINGS,
    CHANNEL_SHORTCUTS,
    CHANNEL_KEYBOARD_LAYOUT,
    CHANNEL_KEYBOARDS,
    CHANNEL_POINTERS,
    CHANNEL_DISPLAYS,
    CHANNEL_ACCESSIBILITY,
    CHANNEL_POWER_MANAGER,
    CHANNEL_PANEL,
    CHANNEL_COUNT,
} channel_id;

enum change_flag {
    CHANGE_SETTINGS = 1 << 0,
    CHANGE_SHORTCUTS = 1 << 1,
    CHANGE_INPUT = 1 << 2,
    CHANGE_THEME = 1 << 3,
    CHANGE_DISPLAYS = 1 << 4,
    CHANGE_X11_SETTINGS = 1 << 5,
    CHANGE_POWER = 1 << 6,
    CHANGE_WINDOW_RULES = 1 << 7,
    CHANGE_ICON_THEME = 1 << 8,
};

const char *channel_name(channel_id id);
unsigned change_flags_for(channel_id id, const char *property);

#endif
