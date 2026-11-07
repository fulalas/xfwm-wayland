#include "x11_motif.h"

#include <stdbool.h>
#include <stddef.h>

#include "wm/desktop.h"

enum {
    HINTS_FUNCTIONS = 1 << 0,
    HINTS_DECORATIONS = 1 << 1,
    FUNC_ALL = 1 << 0,
    FUNC_RESIZE = 1 << 1,
    FUNC_MOVE = 1 << 2,
    FUNC_MINIMIZE = 1 << 3,
    FUNC_MAXIMIZE = 1 << 4,
    FUNC_CLOSE = 1 << 5,
    DECOR_ALL = 1 << 0,
    DECOR_BORDER = 1 << 1,
    DECOR_TITLE = 1 << 3,
};

typedef struct function_cap {
    uint32_t function;
    uint32_t capability;
} function_cap;

static const function_cap function_caps[] = {
    {FUNC_RESIZE, WM_CAP_RESIZE},     {FUNC_MOVE, WM_CAP_MOVE},
    {FUNC_MINIMIZE, WM_CAP_MINIMIZE}, {FUNC_MAXIMIZE, WM_CAP_MAXIMIZE},
    {FUNC_CLOSE, WM_CAP_CLOSE},
};

static uint32_t apply_functions(uint32_t capabilities, uint32_t functions);

uint32_t x11_motif_capabilities(const uint32_t *hints, uint32_t count)
{
    uint32_t capabilities = WM_CAP_ALL;
    if (count < X11_MOTIF_HINT_WORDS) {
        return capabilities;
    }
    uint32_t flags = hints[0];
    uint32_t decorations = hints[2];
    bool framed = (decorations & (DECOR_ALL | DECOR_BORDER | DECOR_TITLE)) != 0;
    if ((flags & HINTS_DECORATIONS) != 0 && !framed) {
        capabilities &= ~(uint32_t) WM_CAP_DECORATED;
    }
    if ((flags & HINTS_FUNCTIONS) != 0) {
        capabilities = apply_functions(capabilities, hints[1]);
    }
    return capabilities;
}

static uint32_t apply_functions(uint32_t capabilities, uint32_t functions)
{
    bool listed_are_removed = (functions & FUNC_ALL) != 0;
    for (size_t i = 0; i < sizeof(function_caps) / sizeof(function_caps[0]); i++) {
        bool listed = (functions & function_caps[i].function) != 0;
        if (listed != listed_are_removed) {
            capabilities |= function_caps[i].capability;
        } else {
            capabilities &= ~function_caps[i].capability;
        }
    }
    return capabilities;
}
