#include "rules.h"

#include <fnmatch.h>
#include <stdlib.h>
#include <string.h>

#include "desktop_state.h"
#include "focus.h"
#include "monitor_move.h"
#include "monitors.h"
#include "placement.h"
#include "states.h"
#include "window_ops.h"

static rule_set copy_rules(const wm_window_rule *rules, size_t count);
static wm_window_rule copy_rule(const wm_window_rule *rule);
static void free_rule(wm_window_rule *rule);
static window_list windows_hit_by_change(const wm_desktop *desktop, const rule_set *old);
static bool contains_rule(const rule_set *set, const wm_window_rule *rule);
static bool rules_equal(const wm_window_rule *a, const wm_window_rule *b);
static bool switches_equal(wm_rule_switch a, wm_rule_switch b);
static bool texts_equal(const char *a, const char *b);
static bool rule_matches(const wm_window_rule *rule, const wm_window *window);
static bool text_matches(const char *pattern, const char *text, wm_rule_match match);
static wm_window_rule effect_for(const rule_set *set, const wm_window *window);
static void merge(wm_window_rule *effect, const wm_window_rule *rule);
static void merge_switch(wm_rule_switch *into, wm_rule_switch from);
static void apply_live(wm_desktop *desktop, wm_window *window, const wm_window_rule *effect);
static void apply_states(wm_desktop *desktop, wm_window *window, const wm_window_rule *effect);
static void add_switch(wm_state_change *change, uint32_t state, wm_rule_switch rule_switch);
static bool workspace_applies(const wm_desktop *desktop, const wm_window *window,
                              const wm_window_rule *effect);
static void place_new(wm_desktop *desktop, wm_window *window, const wm_window_rule *effect);
static void place_live(wm_desktop *desktop, wm_window *window, const wm_window_rule *effect);
static bool find_output(const wm_desktop *desktop, const wm_window_rule *effect, size_t *index);
static void anchor(wm_desktop *desktop, wm_window *window, size_t monitor,
                   const wm_window_rule *effect);

void wm_desktop_set_window_rules(wm_desktop *desktop, const wm_window_rule *rules, size_t count)
{
    rule_set old = desktop->rules;
    desktop->rules = copy_rules(rules, count);
    window_list targets = windows_hit_by_change(desktop, &old);
    for (size_t i = 0; i < targets.count; i++) {
        wm_window_rule effect = effect_for(&desktop->rules, targets.items[i]);
        apply_live(desktop, targets.items[i], &effect);
    }
    window_list_free(&targets);
    rules_free(&old);
    present_flush(desktop);
}

void rules_apply_to_new(wm_desktop *desktop, wm_window *window)
{
    wm_window_rule effect = effect_for(&desktop->rules, window);
    apply_states(desktop, window, &effect);
    if (effect.no_focus.is_set) {
        window->no_focus = effect.no_focus.on;
    }
    if (workspace_applies(desktop, window, &effect)) {
        window->workspace = effect.workspace;
    }
    place_new(desktop, window, &effect);
}

void rules_free(rule_set *set)
{
    for (size_t i = 0; i < set->count; i++) {
        free_rule(&set->items[i]);
    }
    free(set->items);
    set->items = NULL;
    set->count = 0;
}

static rule_set copy_rules(const wm_window_rule *rules, size_t count)
{
    rule_set set = {NULL, 0};
    set.items = count > 0 ? calloc(count, sizeof(*set.items)) : NULL;
    for (size_t i = 0; set.items != NULL && i < count; i++) {
        set.items[set.count++] = copy_rule(&rules[i]);
    }
    return set;
}

static wm_window_rule copy_rule(const wm_window_rule *rule)
{
    wm_window_rule copy = *rule;
    copy.title = rule->title != NULL ? strdup(rule->title) : NULL;
    copy.app_id = rule->app_id != NULL ? strdup(rule->app_id) : NULL;
    copy.output = rule->output != NULL ? strdup(rule->output) : NULL;
    bool lost_text = (rule->title != NULL && copy.title == NULL)
                     || (rule->app_id != NULL && copy.app_id == NULL);
    if (lost_text) {
        free_rule(&copy);
    }
    return copy;
}

static void free_rule(wm_window_rule *rule)
{
    free((char *) rule->title);
    free((char *) rule->app_id);
    free((char *) rule->output);
    rule->title = NULL;
    rule->app_id = NULL;
    rule->output = NULL;
}

static window_list windows_hit_by_change(const wm_desktop *desktop, const rule_set *old)
{
    window_list hit = {NULL, 0, 0};
    const rule_set *current = &desktop->rules;
    for (size_t r = 0; r < current->count; r++) {
        const wm_window_rule *rule = &current->items[r];
        if (contains_rule(old, rule)) {
            continue;
        }
        for (size_t w = 0; w < desktop->stack.count; w++) {
            wm_window *window = desktop->stack.items[w];
            if (!window_list_contains(&hit, window) && rule_matches(rule, window)) {
                window_list_append(&hit, window);
            }
        }
    }
    return hit;
}

static bool contains_rule(const rule_set *set, const wm_window_rule *rule)
{
    for (size_t i = 0; i < set->count; i++) {
        if (rules_equal(&set->items[i], rule)) {
            return true;
        }
    }
    return false;
}

static bool rules_equal(const wm_window_rule *a, const wm_window_rule *b)
{
    return texts_equal(a->title, b->title) && texts_equal(a->app_id, b->app_id)
           && a->match == b->match && a->anchor == b->anchor && a->margin == b->margin
           && texts_equal(a->output, b->output) && a->has_workspace == b->has_workspace
           && a->workspace == b->workspace && switches_equal(a->above, b->above)
           && switches_equal(a->skip_taskbar, b->skip_taskbar)
           && switches_equal(a->no_focus, b->no_focus);
}

static bool switches_equal(wm_rule_switch a, wm_rule_switch b)
{
    return a.is_set == b.is_set && (!a.is_set || a.on == b.on);
}

static bool texts_equal(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return a == b;
    }
    return strcmp(a, b) == 0;
}

static bool rule_matches(const wm_window_rule *rule, const wm_window *window)
{
    if (rule->title == NULL && rule->app_id == NULL) {
        return false;
    }
    return text_matches(rule->title, window->title, rule->match)
           && text_matches(rule->app_id, window->app_id, rule->match);
}

static bool text_matches(const char *pattern, const char *text, wm_rule_match match)
{
    if (pattern == NULL) {
        return true;
    }
    const char *subject = text != NULL ? text : "";
    if (match == WM_MATCH_GLOB) {
        return fnmatch(pattern, subject, 0) == 0;
    }
    return strcmp(pattern, subject) == 0;
}

static wm_window_rule effect_for(const rule_set *set, const wm_window *window)
{
    wm_window_rule effect;
    memset(&effect, 0, sizeof(effect));
    for (size_t i = 0; i < set->count; i++) {
        if (rule_matches(&set->items[i], window)) {
            merge(&effect, &set->items[i]);
        }
    }
    return effect;
}

static void merge(wm_window_rule *effect, const wm_window_rule *rule)
{
    if (rule->anchor != WM_ANCHOR_NONE) {
        effect->anchor = rule->anchor;
        effect->margin = rule->margin;
    }
    if (rule->output != NULL) {
        effect->output = rule->output;
    }
    if (rule->has_workspace) {
        effect->has_workspace = true;
        effect->workspace = rule->workspace;
    }
    merge_switch(&effect->above, rule->above);
    merge_switch(&effect->skip_taskbar, rule->skip_taskbar);
    merge_switch(&effect->no_focus, rule->no_focus);
}

static void merge_switch(wm_rule_switch *into, wm_rule_switch from)
{
    if (from.is_set) {
        *into = from;
    }
}

static void apply_live(wm_desktop *desktop, wm_window *window, const wm_window_rule *effect)
{
    apply_states(desktop, window, effect);
    if (effect->no_focus.is_set) {
        window->no_focus = effect->no_focus.on;
    }
    if (window->no_focus && desktop->focus.focused == window) {
        window_list gone = {NULL, 0, 0};
        window_list_append(&gone, window);
        focus_pass_after_hide(desktop, window, &gone);
        window_list_free(&gone);
    }
    if (workspace_applies(desktop, window, effect) && window->workspace != effect->workspace) {
        window_ops_send_to_workspace(desktop, window, effect->workspace);
    }
    place_live(desktop, window, effect);
}

static void apply_states(wm_desktop *desktop, wm_window *window, const wm_window_rule *effect)
{
    wm_state_change change = {0, 0};
    add_switch(&change, WM_STATE_ABOVE, effect->above);
    add_switch(&change, WM_STATE_SKIP_TASKBAR, effect->skip_taskbar);
    states_apply_change(desktop, window, &change);
}

static void add_switch(wm_state_change *change, uint32_t state, wm_rule_switch rule_switch)
{
    if (rule_switch.is_set) {
        *(rule_switch.on ? &change->set : &change->unset) |= state;
    }
}

static bool workspace_applies(const wm_desktop *desktop, const wm_window *window,
                              const wm_window_rule *effect)
{
    return effect->has_workspace && effect->workspace >= 0
           && effect->workspace < desktop->workspaces.count && !window_is_sticky(window);
}

static void place_new(wm_desktop *desktop, wm_window *window, const wm_window_rule *effect)
{
    size_t target;
    bool has_target = find_output(desktop, effect, &target);
    if (effect->anchor != WM_ANCHOR_NONE) {
        anchor(desktop, window, has_target ? target : monitors_index_of(desktop, window), effect);
    } else if (has_target && target != monitors_index_of(desktop, window)) {
        wm_rect area = monitors_work_area_of(desktop, target);
        placement_place_in(desktop, window, &area);
    }
}

static void place_live(wm_desktop *desktop, wm_window *window, const wm_window_rule *effect)
{
    size_t target;
    bool has_target = find_output(desktop, effect, &target);
    if (has_target) {
        monitor_move_to(desktop, window, target);
    }
    bool keeps_place = window_is_fullscreen(window)
                       || window_has_state(window, WM_STATE_MAXIMIZED);
    if (effect->anchor != WM_ANCHOR_NONE && !keeps_place) {
        anchor(desktop, window, has_target ? target : monitors_index_of(desktop, window), effect);
    }
}

static bool find_output(const wm_desktop *desktop, const wm_window_rule *effect, size_t *index)
{
    return effect->output != NULL && monitors_find_named(desktop, effect->output, index);
}

static void anchor(wm_desktop *desktop, wm_window *window, size_t monitor,
                   const wm_window_rule *effect)
{
    if (monitors_count(desktop) == 0) {
        return;
    }
    wm_rect area = monitors_work_area_of(desktop, monitor);
    placement_anchor(desktop, window, &area, effect->anchor, effect->margin);
    window->tile = TILE_NONE;
    window_placed(window);
    window_mark_dirty(window);
}
