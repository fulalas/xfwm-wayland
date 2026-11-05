#ifndef XFCE_SESSION_EXPORT_H
#define XFCE_SESSION_EXPORT_H

#include "xfce/environment.h"

void session_export(const xfce_session_display *display);
char **session_export_activation_argv(void);
void session_export_update_activation(void);

#endif
