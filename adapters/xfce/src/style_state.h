#ifndef XFCE_STYLE_STATE_H
#define XFCE_STYLE_STATE_H

#include <stdbool.h>

#include "helper_protocol.h"
#include "xfce/environment.h"

typedef struct style_state style_state;

style_state *style_state_create(void);
void style_state_destroy(style_state *state);
xfce_style style_state_current(const style_state *state);
bool style_state_pending_differs(const style_state *state);
void style_state_apply(style_state *state, const helper_message *message);

#endif
