#ifndef XFCE_SHORTCUTS_H
#define XFCE_SHORTCUTS_H

#include <xfconf/xfconf.h>

#include "keymap_store.h"

keymap_store *shortcuts_load(XfconfChannel *channel);

#endif
