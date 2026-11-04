#include "highlight.h"

#include "desktop_state.h"
#include "frame.h"

static bool outline_taken(const wm_desktop *desktop);

void highlight_begin(wm_desktop *desktop, wm_window *window)
{
    desktop->highlighted = window;
}

void highlight_end(wm_desktop *desktop, const wm_window *window)
{
    if (window == NULL || desktop->highlighted != window) {
        return;
    }
    desktop->highlighted = NULL;
    if (!outline_taken(desktop)) {
        present_hide_outline(desktop);
    }
}

void highlight_present(wm_desktop *desktop)
{
    const wm_window *window = desktop->highlighted;
    if (window == NULL || outline_taken(desktop)) {
        return;
    }
    if (!window->visible) {
        present_hide_outline(desktop);
        return;
    }
    wm_rect frame = frame_rect(desktop, window);
    present_show_outline(desktop, &frame);
}

static bool outline_taken(const wm_desktop *desktop)
{
    return moveresize_is_active(desktop) || cycle_is_active(desktop);
}
