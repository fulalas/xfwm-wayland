#ifndef COMPOSITOR_XWAYLAND_STARTUP_H
#define COMPOSITOR_XWAYLAND_STARTUP_H

struct wl_event_loop;

typedef struct xwayland_startup xwayland_startup;
typedef void (*xwayland_startup_done)(void *data);

xwayland_startup *xwayland_startup_create(struct wl_event_loop *loop, const char *dir,
                                          int hold_ms);
void xwayland_startup_destroy(xwayland_startup *startup);
void xwayland_startup_run(xwayland_startup *startup, const char *display,
                          xwayland_startup_done done, void *data);
void xwayland_startup_cancel(xwayland_startup *startup);

#endif
