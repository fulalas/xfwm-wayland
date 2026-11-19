#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "app.h"

enum {
    CONTEXT_MAX = 2,
    INITIAL_CAPACITY = 16,
    TIMER_SLACK_MS = 1,
    MS_PER_SECOND = 1000,
    US_PER_MS = 1000,
    NS_PER_MS = 1000000,
    POLLED_EVENTS = G_IO_IN | G_IO_PRI | G_IO_OUT,
};

static const uint32_t TIMER_SLOT = UINT32_MAX;

typedef enum fd_watch {
    FD_POLLED,
    FD_ALWAYS_READY,
    FD_INVALID,
} fd_watch;

typedef struct watched_fd {
    int fd;
    gushort events;
    fd_watch how;
} watched_fd;

typedef struct context_watch {
    GMainContext *context;
    int epoll_fd;
    int timer_fd;
    GPollFD *fds;
    watched_fd *watched;
    int count;
    int watched_count;
    int capacity;
    gint priority;
    gint64 timer_due_ms;
    bool signals_changes;
    bool active;
    bool prepared;
    bool woke;
    bool stale;
    bool unpollable;
} context_watch;

struct glib_bridge {
    compositor *compositor;
    int epoll_fd;
    bool watching;
    bool woke;
    context_watch contexts[CONTEXT_MAX];
    int count;
};

static void on_fd_ready(void *data);
static bool add_context(glib_bridge *bridge, GMainContext *context, bool signals_changes);
static void release_context(context_watch *watch);
static bool prepare(context_watch *watch);
static bool dispatch(context_watch *watch);
static gint query(context_watch *watch);
static void reserve(context_watch *watch, int capacity);
static bool same_fds(const context_watch *watch);
static void watch_fds(context_watch *watch);
static void refresh_fds(context_watch *watch);
static struct epoll_event epoll_event_for(const context_watch *watch, int slot);
static fd_watch watch_fd(const context_watch *watch, int slot);
static void arm_timer(context_watch *watch, gint timeout_ms);
static void set_timer(const context_watch *watch, gint timeout_ms);
static void collect(glib_bridge *bridge);
static void collect_context(context_watch *watch);
static void note_timer(context_watch *watch);
static void fill_unpollable(context_watch *watch);

glib_bridge *glib_bridge_create(compositor *compositor, GMainContext *quiet_context)
{
    glib_bridge *bridge = calloc(1, sizeof(*bridge));
    if (!bridge) {
        return NULL;
    }
    bridge->compositor = compositor;
    bridge->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (bridge->epoll_fd < 0 || !add_context(bridge, g_main_context_default(), false) ||
        (quiet_context && !add_context(bridge, quiet_context, true))) {
        glib_bridge_destroy(bridge);
        return NULL;
    }
    bridge->watching = compositor_watch_fd(compositor, bridge->epoll_fd, on_fd_ready, bridge);
    if (!bridge->watching) {
        glib_bridge_destroy(bridge);
        return NULL;
    }
    return bridge;
}

void glib_bridge_destroy(glib_bridge *bridge)
{
    if (bridge->watching) {
        compositor_unwatch_fd(bridge->compositor);
    }
    for (int i = 0; i < bridge->count; i++) {
        release_context(&bridge->contexts[i]);
    }
    if (bridge->epoll_fd >= 0) {
        close(bridge->epoll_fd);
    }
    free(bridge);
}

bool glib_bridge_prepare(glib_bridge *bridge)
{
    bool pending = false;

    for (int i = 0; i < bridge->count; i++) {
        context_watch *watch = &bridge->contexts[i];
        watch->prepared = watch->active || !watch->signals_changes || watch->unpollable;
        watch->active = watch->prepared && prepare(watch);
        pending = pending || watch->active;
    }
    return pending;
}

void glib_bridge_dispatch(glib_bridge *bridge)
{
    if (bridge->woke) {
        collect(bridge);
        bridge->woke = false;
    }
    for (int i = 0; i < bridge->count; i++) {
        context_watch *watch = &bridge->contexts[i];
        bool ready = watch->prepared && dispatch(watch);
        watch->active = watch->active || watch->woke || ready;
        watch->woke = false;
    }
}

static void on_fd_ready(void *data)
{
    glib_bridge *bridge = data;

    bridge->woke = true;
}

static bool add_context(glib_bridge *bridge, GMainContext *context, bool signals_changes)
{
    context_watch *watch = &bridge->contexts[bridge->count];

    if (!g_main_context_acquire(context)) {
        return false;
    }
    watch->context = context;
    watch->signals_changes = signals_changes;
    watch->active = true;
    watch->timer_due_ms = -1;
    reserve(watch, INITIAL_CAPACITY);
    watch->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    watch->timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    struct epoll_event context_event = {.events = EPOLLIN, .data.u32 = (uint32_t) bridge->count};
    struct epoll_event timer_event = {.events = EPOLLIN, .data.u32 = TIMER_SLOT};
    bridge->count++;
    return watch->epoll_fd >= 0 && watch->timer_fd >= 0 &&
           epoll_ctl(watch->epoll_fd, EPOLL_CTL_ADD, watch->timer_fd, &timer_event) == 0 &&
           epoll_ctl(bridge->epoll_fd, EPOLL_CTL_ADD, watch->epoll_fd, &context_event) == 0;
}

static void release_context(context_watch *watch)
{
    g_main_context_release(watch->context);
    if (watch->timer_fd >= 0) {
        close(watch->timer_fd);
    }
    if (watch->epoll_fd >= 0) {
        close(watch->epoll_fd);
    }
    g_free(watch->fds);
    g_free(watch->watched);
}

static bool prepare(context_watch *watch)
{
    gboolean ready = g_main_context_prepare(watch->context, &watch->priority);
    gint timeout_ms = query(watch);

    watch_fds(watch);
    arm_timer(watch, timeout_ms);
    return ready || timeout_ms == 0 || watch->unpollable;
}

static bool dispatch(context_watch *watch)
{
    if (watch->unpollable) {
        fill_unpollable(watch);
    }
    bool ready = g_main_context_check(watch->context, watch->priority, watch->fds, watch->count);
    if (ready) {
        g_main_context_dispatch(watch->context);
    }
    return ready;
}

static gint query(context_watch *watch)
{
    gint timeout_ms = -1;
    int needed;

    while ((needed = g_main_context_query(watch->context, watch->priority, &timeout_ms,
                                          watch->fds, watch->capacity)) > watch->capacity) {
        reserve(watch, needed);
    }
    watch->count = needed;
    return timeout_ms;
}

static void reserve(context_watch *watch, int capacity)
{
    watch->fds = g_renew(GPollFD, watch->fds, capacity);
    watch->watched = g_renew(watched_fd, watch->watched, capacity);
    watch->capacity = capacity;
}

static bool same_fds(const context_watch *watch)
{
    if (watch->count != watch->watched_count) {
        return false;
    }
    for (int i = 0; i < watch->count; i++) {
        if (watch->fds[i].fd != watch->watched[i].fd ||
            watch->fds[i].events != watch->watched[i].events) {
            return false;
        }
    }
    return true;
}

static void watch_fds(context_watch *watch)
{
    bool same = same_fds(watch);
    if (same && watch->stale) {
        refresh_fds(watch);
    }
    watch->stale = false;
    if (same) {
        return;
    }
    for (int i = 0; i < watch->watched_count; i++) {
        if (watch->watched[i].how == FD_POLLED) {
            epoll_ctl(watch->epoll_fd, EPOLL_CTL_DEL, watch->watched[i].fd, NULL);
        }
    }
    watch->unpollable = false;
    for (int i = 0; i < watch->count; i++) {
        fd_watch how = watch_fd(watch, i);
        watch->watched[i] = (watched_fd) {watch->fds[i].fd, watch->fds[i].events, how};
        watch->unpollable = watch->unpollable || how != FD_POLLED;
    }
    watch->watched_count = watch->count;
}

static void refresh_fds(context_watch *watch)
{
    for (int i = 0; i < watch->count; i++) {
        if (watch->watched[i].how != FD_POLLED) {
            continue;
        }
        struct epoll_event event = epoll_event_for(watch, i);
        if (epoll_ctl(watch->epoll_fd, EPOLL_CTL_MOD, watch->fds[i].fd, &event) != 0) {
            watch->watched[i].how = watch_fd(watch, i);
            watch->unpollable = watch->unpollable || watch->watched[i].how != FD_POLLED;
        }
    }
}

static struct epoll_event epoll_event_for(const context_watch *watch, int slot)
{
    return (struct epoll_event) {
        .events = (uint32_t) (watch->fds[slot].events & POLLED_EVENTS),
        .data.u32 = (uint32_t) slot,
    };
}

static fd_watch watch_fd(const context_watch *watch, int slot)
{
    struct epoll_event event = epoll_event_for(watch, slot);

    if (epoll_ctl(watch->epoll_fd, EPOLL_CTL_ADD, watch->fds[slot].fd, &event) == 0) {
        return FD_POLLED;
    }
    return errno == EPERM ? FD_ALWAYS_READY : FD_INVALID;
}

static void arm_timer(context_watch *watch, gint timeout_ms)
{
    if (timeout_ms == 0) {
        return;
    }
    if (timeout_ms < 0) {
        if (watch->timer_due_ms >= 0) {
            set_timer(watch, 0);
            watch->timer_due_ms = -1;
        }
        return;
    }
    gint64 due_ms = g_get_monotonic_time() / US_PER_MS + timeout_ms;
    gint64 drift_ms = due_ms - watch->timer_due_ms;
    if (watch->timer_due_ms >= 0 && drift_ms >= -TIMER_SLACK_MS && drift_ms <= TIMER_SLACK_MS) {
        return;
    }
    set_timer(watch, timeout_ms);
    watch->timer_due_ms = due_ms;
}

static void set_timer(const context_watch *watch, gint timeout_ms)
{
    struct itimerspec spec = {
        .it_value = {
            .tv_sec = timeout_ms / MS_PER_SECOND,
            .tv_nsec = (long) (timeout_ms % MS_PER_SECOND) * NS_PER_MS,
        },
    };

    timerfd_settime(watch->timer_fd, 0, &spec, NULL);
}

static void collect(glib_bridge *bridge)
{
    struct epoll_event events[CONTEXT_MAX];
    int count = epoll_wait(bridge->epoll_fd, events, CONTEXT_MAX, 0);

    for (int i = 0; i < count; i++) {
        collect_context(&bridge->contexts[events[i].data.u32]);
    }
}

static void collect_context(context_watch *watch)
{
    int capacity = watch->watched_count + 1;
    struct epoll_event *events = g_newa(struct epoll_event, capacity);
    int count = epoll_wait(watch->epoll_fd, events, capacity, 0);

    for (int i = 0; i < count; i++) {
        uint32_t slot = events[i].data.u32;
        watch->woke = true;
        watch->stale = true;
        if (slot == TIMER_SLOT) {
            note_timer(watch);
        } else if (watch->prepared && (int) slot < watch->count) {
            watch->fds[slot].revents = (gushort) events[i].events;
        }
    }
}

static void note_timer(context_watch *watch)
{
    uint64_t expirations;

    if (read(watch->timer_fd, &expirations, sizeof(expirations)) > 0) {
        watch->timer_due_ms = -1;
    }
}

static void fill_unpollable(context_watch *watch)
{
    for (int i = 0; i < watch->count; i++) {
        if (watch->watched[i].how == FD_ALWAYS_READY) {
            watch->fds[i].revents = watch->fds[i].events & (G_IO_IN | G_IO_OUT);
        } else if (watch->watched[i].how == FD_INVALID) {
            watch->fds[i].revents = G_IO_NVAL;
        }
    }
}
