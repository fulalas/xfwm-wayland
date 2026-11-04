#ifndef XFCE_CHANGES_H
#define XFCE_CHANGES_H

typedef enum channel_id {
    CHANNEL_XFWM4,
    CHANNEL_XSETTINGS,
    CHANNEL_SHORTCUTS,
    CHANNEL_KEYBOARD_LAYOUT,
    CHANNEL_KEYBOARDS,
    CHANNEL_POINTERS,
    CHANNEL_COUNT,
} channel_id;

enum change_flag {
    CHANGE_SETTINGS = 1 << 0,
    CHANGE_SHORTCUTS = 1 << 1,
    CHANGE_INPUT = 1 << 2,
    CHANGE_THEME = 1 << 3,
};

const char *channel_name(channel_id id);
unsigned change_flags_for(channel_id id, const char *property);

#endif
