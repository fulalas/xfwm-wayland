#ifndef COMPOSITOR_X11_MOTIF_H
#define COMPOSITOR_X11_MOTIF_H

#include <stdint.h>

enum {
    X11_MOTIF_HINT_WORDS = 5,
};

uint32_t x11_motif_capabilities(const uint32_t *hints, uint32_t count);

#endif
