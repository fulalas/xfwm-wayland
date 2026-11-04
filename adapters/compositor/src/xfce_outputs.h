#ifndef COMPOSITOR_XFCE_OUTPUTS_H
#define COMPOSITOR_XFCE_OUTPUTS_H

#include "server.h"

xfce_outputs *xfce_outputs_create(compositor *server);
void xfce_outputs_destroy(xfce_outputs *outputs);
void xfce_outputs_refresh(xfce_outputs *outputs);

#endif
