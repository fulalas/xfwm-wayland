#include "mode_choice.h"

#include <stdlib.h>

static int preferred_index(const mode_options *options);
static bool same_size(const display_mode *a, const display_mode *b);
static bool is_closer(const display_mode *candidate, const display_mode *best,
                      const display_mode *wanted);

int mode_choice_pick(const mode_options *options, const display_mode *wanted)
{
    if (wanted->width <= 0 || wanted->height <= 0) {
        return preferred_index(options);
    }
    int best = MODE_CHOICE_NONE;
    for (size_t i = 0; i < options->count; i++) {
        const display_mode *candidate = &options->options[i].mode;
        if (same_size(candidate, wanted) &&
            (best == MODE_CHOICE_NONE ||
             is_closer(candidate, &options->options[best].mode, wanted))) {
            best = (int) i;
        }
    }
    return best;
}

static int preferred_index(const mode_options *options)
{
    for (size_t i = 0; i < options->count; i++) {
        if (options->options[i].is_preferred) {
            return (int) i;
        }
    }
    return options->count > 0 ? 0 : MODE_CHOICE_NONE;
}

static bool same_size(const display_mode *a, const display_mode *b)
{
    return a->width == b->width && a->height == b->height;
}

static bool is_closer(const display_mode *candidate, const display_mode *best,
                      const display_mode *wanted)
{
    if (wanted->refresh_mhz <= 0) {
        return candidate->refresh_mhz > best->refresh_mhz;
    }
    return abs(candidate->refresh_mhz - wanted->refresh_mhz) <
           abs(best->refresh_mhz - wanted->refresh_mhz);
}
