#ifndef COMPOSITOR_SESSION_APPLY_H
#define COMPOSITOR_SESSION_APPLY_H

#include <stdbool.h>

#include "session_record.h"
#include "wm/desktop.h"

session_record session_record_from_view(const wm_window_view *view);
bool session_record_same_state(const session_record *a, const session_record *b);
void session_apply_launch(const session_record *record, wm_window_desc *desc);
void session_apply_recovery(const session_record *record, wm_window_desc *desc);

#endif
