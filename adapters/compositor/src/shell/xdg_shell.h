#ifndef COMPOSITOR_XDG_SHELL_H
#define COMPOSITOR_XDG_SHELL_H

#include "server/server.h"

xdg_shell *xdg_shell_create(compositor *server);
void xdg_shell_destroy(xdg_shell *shell);

#endif
