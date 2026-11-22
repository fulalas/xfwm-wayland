#ifndef XFCE_SYSTEM_KEYMAP_H
#define XFCE_SYSTEM_KEYMAP_H

typedef enum system_keymap_field {
    SYSTEM_KEYMAP_LAYOUT,
    SYSTEM_KEYMAP_VARIANT,
    SYSTEM_KEYMAP_MODEL,
    SYSTEM_KEYMAP_OPTIONS,
    SYSTEM_KEYMAP_FIELDS,
} system_keymap_field;

typedef struct system_keymap {
    char *fields[SYSTEM_KEYMAP_FIELDS];
} system_keymap;

void system_keymap_read_vconsole(system_keymap *keymap, const char *text);
void system_keymap_read_xorg(system_keymap *keymap, const char *text);
system_keymap system_keymap_choose(system_keymap *vconsole, system_keymap *xorg,
                                    char **environment);
void system_keymap_clear(system_keymap *keymap);

#endif
