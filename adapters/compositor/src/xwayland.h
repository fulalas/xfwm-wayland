#ifndef COMPOSITOR_XWAYLAND_H
#define COMPOSITOR_XWAYLAND_H

#include "output.h"
#include "server.h"

xwayland_shell *xwayland_shell_create(compositor *server);
void xwayland_shell_destroy(xwayland_shell *shell);
const char *xwayland_shell_display_name(const xwayland_shell *shell);
wm_rect xwayland_shell_apply_struts(const xwayland_shell *shell, const output *output,
                                    wm_rect usable);

#endif
