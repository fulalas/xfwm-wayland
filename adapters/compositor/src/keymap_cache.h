#ifndef COMPOSITOR_KEYMAP_CACHE_H
#define COMPOSITOR_KEYMAP_CACHE_H

#include "compositor/services.h"

struct xkb_keymap;

typedef struct keymap_cache keymap_cache;

keymap_cache *keymap_cache_create(void);
void keymap_cache_destroy(keymap_cache *cache);
void keymap_cache_update(keymap_cache *cache, const keyboard_prefs *prefs);
struct xkb_keymap *keymap_cache_keymap(const keymap_cache *cache);

#endif
