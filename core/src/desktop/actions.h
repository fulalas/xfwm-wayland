#ifndef DESKTOP_ACTIONS_H
#define DESKTOP_ACTIONS_H

#include <stdbool.h>

#include "wm/desktop.h"

wm_dispatch actions_handle_key(wm_desktop *desktop, const wm_key_event *event);
void actions_run_global(wm_desktop *desktop, wm_action action);

#endif
