#include "display_modules.h"

#include "displays.h"
#include "lid_switch.h"
#include "xfce_output_heads.h"
#include "xfce_outputs.h"

bool display_modules_create(compositor *c)
{
    c->displays = displays_create(c);
    c->lid_switch = lid_switch_create(c);
    c->xfce_output_heads = xfce_output_heads_create(c);
    c->xfce_outputs = xfce_outputs_create(c);
    return c->displays != NULL && c->lid_switch != NULL && c->xfce_output_heads != NULL &&
           c->xfce_outputs != NULL;
}

void display_modules_destroy(compositor *c)
{
    if (c->xfce_outputs != NULL) {
        xfce_outputs_destroy(c->xfce_outputs);
        c->xfce_outputs = NULL;
    }
    if (c->xfce_output_heads != NULL) {
        xfce_output_heads_destroy(c->xfce_output_heads);
        c->xfce_output_heads = NULL;
    }
    if (c->lid_switch != NULL) {
        lid_switch_destroy(c->lid_switch);
        c->lid_switch = NULL;
    }
    if (c->displays != NULL) {
        displays_destroy(c->displays);
        c->displays = NULL;
    }
}
