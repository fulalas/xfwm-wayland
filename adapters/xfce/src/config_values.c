#include "config_values.h"

#include <glib.h>
#include <stdlib.h>

#include "accelerator.h"

typedef struct value_name {
    const char *name;
    int value;
} value_name;

static const value_name activate_actions[] = {
    { "bring", WM_ACTIVATE_BRING },
    { "switch", WM_ACTIVATE_SWITCH },
};

static const value_name double_click_actions[] = {
    { "shade", WM_DOUBLE_CLICK_SHADE },
    { "hide", WM_DOUBLE_CLICK_HIDE },
    { "maximize", WM_DOUBLE_CLICK_MAXIMIZE },
    { "fill", WM_DOUBLE_CLICK_FILL },
    { "above", WM_DOUBLE_CLICK_ABOVE },
};

static const value_name placement_modes[] = {
    { "mouse", WM_PLACE_MOUSE },
};

static const value_name alignments[] = {
    { "left", WM_ALIGN_LEFT },
    { "right", WM_ALIGN_RIGHT },
};

static const value_name title_shadows[] = {
    { "true", WM_TITLE_SHADOW_UNDER },
    { "under", WM_TITLE_SHADOW_UNDER },
    { "frame", WM_TITLE_SHADOW_FRAME },
};

static const value_name vblank_modes[] = {
    { "off", WM_VBLANK_OFF },
    { "adaptive", WM_VBLANK_ADAPTIVE },
};

typedef struct value_table {
    const value_name *names;
    size_t count;
    int fallback;
} value_table;

#define TABLE(names, fallback) { names, G_N_ELEMENTS(names), fallback }

static const value_table activate_table = TABLE(activate_actions, WM_ACTIVATE_NONE);
static const value_table double_click_table = TABLE(double_click_actions, WM_DOUBLE_CLICK_NONE);
static const value_table placement_table = TABLE(placement_modes, WM_PLACE_CENTER);
static const value_table alignment_table = TABLE(alignments, WM_ALIGN_CENTER);
static const value_table title_shadow_table = TABLE(title_shadows, WM_TITLE_SHADOW_NONE);
static const value_table vblank_table = TABLE(vblank_modes, WM_VBLANK_AUTO);

static int lookup_value(const value_table *table, const char *text);

bool config_bool(const char *text)
{
    return g_ascii_strcasecmp(text, "true") == 0;
}

int config_int(const char *text)
{
    return atoi(text);
}

wm_activate_action config_activate_action(const char *text)
{
    return lookup_value(&activate_table, text);
}

wm_double_click_action config_double_click_action(const char *text)
{
    return lookup_value(&double_click_table, text);
}

wm_placement_mode config_placement_mode(const char *text)
{
    return lookup_value(&placement_table, text);
}

wm_alignment config_alignment(const char *text)
{
    return lookup_value(&alignment_table, text);
}

wm_title_shadow config_title_shadow(const char *text)
{
    return lookup_value(&title_shadow_table, text);
}

wm_vblank_mode config_vblank_mode(const char *text)
{
    return lookup_value(&vblank_table, text);
}

uint32_t config_easy_click(const char *text)
{
    if (config_bool(text)) {
        return WM_MOD_ALT;
    }
    return accelerator_modifier_from_name(text);
}

static int lookup_value(const value_table *table, const char *text)
{
    for (size_t i = 0; i < table->count; i++) {
        if (g_ascii_strcasecmp(text, table->names[i].name) == 0) {
            return table->names[i].value;
        }
    }
    return table->fallback;
}
