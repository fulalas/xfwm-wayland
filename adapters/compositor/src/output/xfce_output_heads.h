#ifndef COMPOSITOR_XFCE_OUTPUT_HEADS_H
#define COMPOSITOR_XFCE_OUTPUT_HEADS_H

#include <stdbool.h>

#include "server/server.h"

struct wl_client;

xfce_output_heads *xfce_output_heads_create(compositor *server);
void xfce_output_heads_destroy(xfce_output_heads *heads);
void xfce_output_heads_sync(xfce_output_heads *heads);
bool xfce_output_heads_has_client(const xfce_output_heads *heads, const struct wl_client *client);

#endif
