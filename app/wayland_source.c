#include "app.h"

typedef struct wayland_source {
    GSource base;
    compositor *compositor;
} wayland_source;

static gboolean prepare(GSource *source, gint *timeout_ms);
static gboolean dispatch(GSource *source, GSourceFunc callback, gpointer data);

static GSourceFuncs wayland_source_funcs = {
    .prepare = prepare,
    .dispatch = dispatch,
};

GSource *app_wayland_source_new(compositor *compositor)
{
    GSource *source = g_source_new(&wayland_source_funcs, sizeof(wayland_source));
    wayland_source *wayland = (wayland_source *) source;

    wayland->compositor = compositor;
    g_source_add_unix_fd(source, compositor_event_fd(compositor), G_IO_IN | G_IO_ERR | G_IO_HUP);
    g_source_set_name(source, "wayland");
    return source;
}

static gboolean prepare(GSource *source, gint *timeout_ms)
{
    wayland_source *wayland = (wayland_source *) source;

    compositor_flush(wayland->compositor);
    *timeout_ms = -1;
    return FALSE;
}

static gboolean dispatch(GSource *source, GSourceFunc callback, gpointer data)
{
    wayland_source *wayland = (wayland_source *) source;

    (void) callback;
    (void) data;
    compositor_dispatch(wayland->compositor);
    return G_SOURCE_CONTINUE;
}
