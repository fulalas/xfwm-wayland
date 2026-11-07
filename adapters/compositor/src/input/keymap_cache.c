#include <stdlib.h>
#include <string.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

#include "keymap_cache.h"

struct keymap_cache {
    struct xkb_context *context;
    struct xkb_keymap *keymap;
    keyboard_prefs rules;
};

static bool same_rules(const keyboard_prefs *a, const keyboard_prefs *b);
static struct xkb_keymap *compile_keymap(struct xkb_context *context,
                                         const keyboard_prefs *prefs);
static const char *rule_or_default(const char *rule);

keymap_cache *keymap_cache_create(void)
{
    keymap_cache *cache = calloc(1, sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (cache->context == NULL) {
        wlr_log(WLR_ERROR, "cannot create the xkb context");
    }
    return cache;
}

void keymap_cache_destroy(keymap_cache *cache)
{
    if (cache->keymap != NULL) {
        xkb_keymap_unref(cache->keymap);
    }
    if (cache->context != NULL) {
        xkb_context_unref(cache->context);
    }
    free(cache);
}

void keymap_cache_update(keymap_cache *cache, const keyboard_prefs *prefs)
{
    if (cache->keymap != NULL && same_rules(&cache->rules, prefs)) {
        return;
    }
    if (cache->keymap != NULL) {
        xkb_keymap_unref(cache->keymap);
    }
    cache->keymap = compile_keymap(cache->context, prefs);
    cache->rules = *prefs;
}

struct xkb_keymap *keymap_cache_keymap(const keymap_cache *cache)
{
    return cache->keymap;
}

static bool same_rules(const keyboard_prefs *a, const keyboard_prefs *b)
{
    return strcmp(a->model, b->model) == 0 && strcmp(a->layout, b->layout) == 0 &&
           strcmp(a->variant, b->variant) == 0 && strcmp(a->options, b->options) == 0;
}

static struct xkb_keymap *compile_keymap(struct xkb_context *context,
                                         const keyboard_prefs *prefs)
{
    if (context == NULL) {
        return NULL;
    }
    struct xkb_rule_names names = {
        .model = rule_or_default(prefs->model),
        .layout = rule_or_default(prefs->layout),
        .variant = rule_or_default(prefs->variant),
        .options = rule_or_default(prefs->options),
    };
    struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, &names, 0);
    if (keymap == NULL) {
        wlr_log(WLR_ERROR, "cannot load keymap layout '%s', using the default", prefs->layout);
        keymap = xkb_keymap_new_from_names(context, NULL, 0);
    }
    return keymap;
}

static const char *rule_or_default(const char *rule)
{
    return rule[0] != '\0' ? rule : NULL;
}
