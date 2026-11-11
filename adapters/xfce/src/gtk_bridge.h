#ifndef XFCE_GTK_BRIDGE_H
#define XFCE_GTK_BRIDGE_H

#include "compositor/services.h"
#include "xfce/environment.h"

typedef struct bridge_listener {
    void *self;
    void (*style_changed)(void *self);
} bridge_listener;

typedef struct gtk_bridge gtk_bridge;

gtk_bridge *gtk_bridge_create(const bridge_listener *listener);
void gtk_bridge_destroy(gtk_bridge *bridge);
void gtk_bridge_start(gtk_bridge *bridge);
void gtk_bridge_forget_icons(gtk_bridge *bridge);
xfce_style gtk_bridge_style(const gtk_bridge *bridge);
void gtk_bridge_lookup_icon(gtk_bridge *bridge, const icon_request *request);
void gtk_bridge_cancel_icons(gtk_bridge *bridge, void *context);
void gtk_bridge_ask_terminate(gtk_bridge *bridge, const terminate_request *request);
void gtk_bridge_cancel_terminate(gtk_bridge *bridge, void *context);

#endif
