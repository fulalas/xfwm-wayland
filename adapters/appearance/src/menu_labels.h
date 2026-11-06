#ifndef APPEARANCE_MENU_LABELS_H
#define APPEARANCE_MENU_LABELS_H

#include "wm/views.h"

const char *appearance_menu_label_msgid(wm_menu_label label);
char *appearance_menu_item_text(const wm_menu_level *level, size_t index);
char *appearance_menu_item_text_with_mnemonic(const wm_menu_level *level, size_t index,
                                              int *mnemonic_at);
uint32_t appearance_menu_item_mnemonic(const wm_menu_level *level, size_t index);
char *appearance_strip_mnemonics(const char *label, int *mnemonic_at);

#endif
