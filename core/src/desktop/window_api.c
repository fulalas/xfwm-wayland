#include "desktop_state.h"
#include "present.h"
#include "stack.h"
#include "transients.h"
#include "urgency.h"

static void refresh_type(wm_desktop *desktop, wm_window *window);

void wm_window_set_title(wm_desktop *desktop, wm_window *window, const char *title)
{
    window_set_title(window, title);
    window->decor_valid = false;
    present_flush(desktop);
}

void wm_window_set_app_id(wm_desktop *desktop, wm_window *window, const char *app_id)
{
    window_set_app_id(window, app_id);
    present_flush(desktop);
}

void wm_window_set_type(wm_desktop *desktop, wm_window *window, wm_window_type type)
{
    window->requested_type = type;
    refresh_type(desktop, window);
    present_flush(desktop);
}

void wm_window_set_capabilities(wm_desktop *desktop, wm_window *window, uint32_t capabilities)
{
    window->capabilities = capabilities;
    window_update_flags(window);
    present_flush(desktop);
}

void wm_window_set_size_hints(wm_desktop *desktop, wm_window *window, const wm_size_hints *hints)
{
    window->hints = *hints;
    window_update_flags(window);
    present_flush(desktop);
}

void wm_window_set_parent(wm_desktop *desktop, wm_window *window, wm_window *parent)
{
    bool known = parent == NULL || window_list_contains(&desktop->stack, parent);
    if (known && transient_parent_allowed(window, parent)) {
        window->parent = parent;
        refresh_type(desktop, window);
    }
    present_flush(desktop);
}

void wm_window_set_opacity(wm_desktop *desktop, wm_window *window, double opacity)
{
    window->opacity = window_opacity_from_double(opacity);
    window_mark_dirty(window);
    present_flush(desktop);
}

void wm_window_size_committed(wm_desktop *desktop, wm_window *window, wm_size size)
{
    wm_rect *client = &window->client;
    const wm_rect *configured = &window->configured;
    if (window->moving_edges & WM_EDGE_LEFT) {
        client->x = configured->x + configured->width - size.width;
    }
    if (window->moving_edges & WM_EDGE_TOP) {
        client->y = configured->y + configured->height - size.height;
    }
    client->width = size.width;
    client->height = size.height;
    window->configured = *client;
    window_mark_dirty(window);
    present_flush(desktop);
}

void wm_window_demand_attention(wm_desktop *desktop, wm_window *window)
{
    urgency_set(desktop, window);
    present_flush(desktop);
}

void wm_window_clear_attention(wm_desktop *desktop, wm_window *window)
{
    urgency_clear(desktop, window);
    present_flush(desktop);
}

void wm_window_not_responding(wm_desktop *desktop, wm_window *window)
{
    if (window->close_requested) {
        desktop->ports.commands.ask_terminate(desktop->ports.commands.self, window->handle);
    }
    present_flush(desktop);
}

void wm_window_icon_changed(wm_desktop *desktop, wm_window *window)
{
    window->has_icon = true;
    window->decor_valid = false;
    window_mark_dirty(window);
    present_flush(desktop);
}

static void refresh_type(wm_desktop *desktop, wm_window *window)
{
    window_apply_type(window);
    window_update_flags(window);
    stack_update_layers(desktop);
}
