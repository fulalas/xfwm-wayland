#include "app.h"

static void configure(void *self, void *window, const wm_rect *client);
static void close_window(void *self, void *window);
static void kill_window(void *self, void *window);
static void ask_terminate(void *self, void *window);
static void warp_pointer(void *self, wm_point point);
static void spawn(void *self, const char *command);
static void logout(void *self);
static void store_workspace_count(void *self, int count);
static void schedule_timer(void *self, uint32_t delay_ms);

wm_commands app_commands(app *app)
{
    return (wm_commands) {
        .self = app,
        .configure = configure,
        .close = close_window,
        .kill = kill_window,
        .ask_terminate = ask_terminate,
        .warp_pointer = warp_pointer,
        .spawn = spawn,
        .logout = logout,
        .store_workspace_count = store_workspace_count,
        .schedule_timer = schedule_timer,
    };
}

static void configure(void *self, void *window, const wm_rect *client)
{
    app *app = self;

    compositor_configure_window(app->compositor, window, client);
}

static void close_window(void *self, void *window)
{
    app *app = self;

    compositor_close_window(app->compositor, window);
}

static void kill_window(void *self, void *window)
{
    app *app = self;

    compositor_kill_window(app->compositor, window);
}

static void ask_terminate(void *self, void *window)
{
    app *app = self;

    compositor_ask_terminate(app->compositor, window);
}

static void warp_pointer(void *self, wm_point point)
{
    app *app = self;

    compositor_warp_pointer(app->compositor, point);
}

static void spawn(void *self, const char *command)
{
    app *app = self;

    xfce_environment_spawn(app->xfce, command);
}

static void logout(void *self)
{
    app *app = self;

    xfce_environment_logout(app->xfce);
}

static void store_workspace_count(void *self, int count)
{
    app *app = self;

    xfce_environment_store_workspace_count(app->xfce, count);
}

static void schedule_timer(void *self, uint32_t delay_ms)
{
    app *app = self;

    compositor_schedule_timer(app->compositor, delay_ms);
}
