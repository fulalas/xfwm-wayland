#include "cycle_list.h"

#include "desktop_state.h"
#include "focus.h"
#include "transients.h"
#include "visibility.h"

static bool passes_filters(const wm_desktop *desktop, const wm_window *window, unsigned mask);
static bool passes_application(const wm_window *window, const cycle_candidate *candidate);
static bool skipped_by_apps_only(const wm_window *window, wm_window *const *items, size_t count);
static bool has_listed_modal(const wm_window *window, wm_window *const *items, size_t count);

unsigned cycle_list_range(const wm_desktop *desktop)
{
    const wm_cycle_config *config = &desktop->config.cycle;
    unsigned range = 0;
    if (config->hidden) {
        range |= SEARCH_INCLUDE_HIDDEN;
    }
    if (!config->minimum) {
        range |= SEARCH_INCLUDE_SKIP;
    }
    if (config->workspaces) {
        range |= SEARCH_INCLUDE_ALL_WORKSPACES;
    }
    return range;
}

size_t cycle_list_build(const wm_desktop *desktop, wm_window *start, wm_window **items)
{
    const window_list *mru = &desktop->mru;
    size_t first = window_list_index_of(mru, start);
    size_t count = 0;
    for (size_t i = 0; i < mru->count && count < WM_CYCLE_ITEM_MAX; i++) {
        wm_window *window = mru->items[(first + i) % mru->count];
        unsigned mask = cycle_list_range(desktop);
        if (transient_is_modal(window)) {
            mask |= SEARCH_INCLUDE_SKIP;
        }
        cycle_candidate candidate = {NULL, mask};
        if (!cycle_list_selects(desktop, window, &candidate)) {
            continue;
        }
        if (desktop->config.cycle.apps_only && skipped_by_apps_only(window, items, count)) {
            continue;
        }
        items[count++] = window;
    }
    return count;
}

bool cycle_list_selects(const wm_desktop *desktop, const wm_window *window,
                        const cycle_candidate *candidate)
{
    if (!passes_application(window, candidate)) {
        return false;
    }
    if (!passes_filters(desktop, window, candidate->mask)) {
        return false;
    }
    if (candidate->mask & SEARCH_NORMAL_ONLY) {
        return window->type == WM_TYPE_NORMAL;
    }
    return window_is_regular_focusable(window);
}

static bool passes_filters(const wm_desktop *desktop, const wm_window *window, unsigned mask)
{
    if (!focus_accepts(desktop, window)) {
        return false;
    }
    if (!(mask & SEARCH_INCLUDE_HIDDEN) && window_is_minimized(window)) {
        return false;
    }
    if (!(mask & SEARCH_INCLUDE_ALL_WORKSPACES)
        && !visibility_on_current_workspace(desktop, window)) {
        return false;
    }
    uint32_t skip = WM_STATE_SKIP_PAGER | WM_STATE_SKIP_TASKBAR;
    return (mask & SEARCH_INCLUDE_SKIP) || !window_has_state(window, skip);
}

static bool passes_application(const wm_window *window, const cycle_candidate *candidate)
{
    if (candidate->reference == NULL) {
        return true;
    }
    bool same = transient_same_application(window, candidate->reference);
    if ((candidate->mask & SEARCH_SAME_APPLICATION) && !same) {
        return false;
    }
    return !((candidate->mask & SEARCH_DIFFERENT_APPLICATION) && same);
}

static bool skipped_by_apps_only(const wm_window *window, wm_window *const *items, size_t count)
{
    if (window_is_dialog(window)) {
        return window->parent != NULL && !transient_is_modal(window);
    }
    if (window->type != WM_TYPE_NORMAL) {
        return true;
    }
    return has_listed_modal(window, items, count);
}

static bool has_listed_modal(const wm_window *window, wm_window *const *items, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (transient_is_modal_for(items[i], window)) {
            return true;
        }
    }
    return false;
}
