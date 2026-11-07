#include "session_apply.h"

static uint32_t states_of(const wm_window_view *view);

session_record session_record_from_view(const wm_window_view *view)
{
    return (session_record){
        .geometry = view->restore_client,
        .states = states_of(view),
        .has_workspace = !view->sticky,
        .workspace = view->sticky ? -1 : view->workspace,
    };
}

bool session_record_same_state(const session_record *a, const session_record *b)
{
    return wm_rect_equals(&a->geometry, &b->geometry) && a->states == b->states &&
           a->has_workspace == b->has_workspace && a->workspace == b->workspace;
}

void session_apply_launch(const session_record *record, wm_window_desc *desc)
{
    desc->geometry = record->geometry;
    desc->has_position = true;
    desc->states |= record->states & ~(uint32_t) WM_STATE_MINIMIZED;
}

void session_apply_recovery(const session_record *record, wm_window_desc *desc)
{
    desc->geometry = record->geometry;
    desc->has_position = true;
    desc->states |= record->states;
    desc->has_workspace = record->has_workspace;
    desc->workspace = record->workspace;
}

static uint32_t states_of(const wm_window_view *view)
{
    uint32_t states = 0;
    states |= view->maximized_horizontally ? WM_STATE_MAXIMIZED_HORZ : 0;
    states |= view->maximized_vertically ? WM_STATE_MAXIMIZED_VERT : 0;
    states |= view->fullscreen ? WM_STATE_FULLSCREEN : 0;
    states |= view->minimized ? WM_STATE_MINIMIZED : 0;
    states |= view->shaded ? WM_STATE_SHADED : 0;
    states |= view->sticky ? WM_STATE_STICKY : 0;
    states |= view->above ? WM_STATE_ABOVE : 0;
    states |= view->below ? WM_STATE_BELOW : 0;
    return states;
}
