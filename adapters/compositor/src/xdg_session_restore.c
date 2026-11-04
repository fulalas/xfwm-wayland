#include "session_apply.h"
#include "xdg-session-management-v1-protocol.h"
#include "xdg_session_private.h"

static session_toplevel *pending_restore(const xdg_sessions *sessions,
                                         const struct wlr_xdg_toplevel *toplevel);

wm_size xdg_sessions_initial_size(const xdg_sessions *sessions,
                                  const struct wlr_xdg_toplevel *toplevel)
{
    const session_toplevel *tracked = pending_restore(sessions, toplevel);
    if (tracked == NULL) {
        return (wm_size){0, 0};
    }
    return (wm_size){tracked->restore.geometry.width, tracked->restore.geometry.height};
}

void xdg_sessions_apply_restore(xdg_sessions *sessions, const struct wlr_xdg_toplevel *toplevel,
                           wm_window_desc *desc)
{
    session_toplevel *tracked = pending_restore(sessions, toplevel);
    if (tracked == NULL) {
        return;
    }
    tracked->restore_pending = false;
    if (tracked->session->reason == XDG_SESSION_MANAGER_V1_REASON_LAUNCH) {
        session_apply_launch(&tracked->restore, desc);
    } else {
        session_apply_recovery(&tracked->restore, desc);
    }
}

static session_toplevel *pending_restore(const xdg_sessions *sessions,
                                         const struct wlr_xdg_toplevel *toplevel)
{
    if (sessions == NULL) {
        return NULL;
    }
    session_toplevel *tracked = session_toplevel_of(sessions, toplevel);
    return tracked != NULL && tracked->restore_pending ? tracked : NULL;
}
