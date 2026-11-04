#include "dispatcher.h"

#include <glib.h>

struct dispatcher {
    dispatch_target target;
    unsigned pending;
    guint idle_id;
};

static gboolean on_idle(void *data);

dispatcher *dispatcher_create(const dispatch_target *target)
{
    dispatcher *dispatcher = g_new0(struct dispatcher, 1);

    dispatcher->target = *target;
    return dispatcher;
}

void dispatcher_destroy(dispatcher *dispatcher)
{
    if (dispatcher->idle_id) {
        g_source_remove(dispatcher->idle_id);
    }
    g_free(dispatcher);
}

void dispatcher_post(dispatcher *dispatcher, unsigned flags)
{
    if (flags == 0) {
        return;
    }
    dispatcher->pending |= flags;
    if (!dispatcher->idle_id) {
        dispatcher->idle_id = g_idle_add(on_idle, dispatcher);
    }
}

static gboolean on_idle(void *data)
{
    dispatcher *dispatcher = data;
    unsigned flags = dispatcher->pending;

    dispatcher->pending = 0;
    dispatcher->idle_id = 0;
    dispatcher->target.run(dispatcher->target.self, flags);
    return G_SOURCE_REMOVE;
}
