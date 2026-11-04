#include "display_batch.h"

#include "display_policy.h"
#include "output_setting.h"

void display_batch_collect(const outputs *outs, display_batch *batch)
{
    batch->count = 0;
    size_t known = outs != NULL ? outputs_known_count(outs) : 0;
    for (size_t i = 0; i < known && batch->count < DISPLAY_MAX; i++) {
        output *o = outputs_known_at(outs, i);
        batch->outputs[batch->count] = o;
        batch->states[batch->count] = (display_state){
            .identity = output_display_identity(o),
            .setting = output_saved_setting(o),
            .is_primary = output_display_of(o)->is_primary,
        };
        batch->count++;
    }
}

bool display_batch_has_pending(const display_batch *batch)
{
    for (size_t i = 0; i < batch->count; i++) {
        if (output_display_of(batch->outputs[i])->is_pending) {
            return true;
        }
    }
    return false;
}

void display_batch_decide(const display_batch *batch, bool lid_closed, bool *enabled)
{
    display_wish wishes[DISPLAY_MAX];
    for (size_t i = 0; i < batch->count; i++) {
        const output_display *display = output_display_of(batch->outputs[i]);
        wishes[i] = (display_wish){display->is_internal, display->wants_enabled};
    }
    display_wishes input = {wishes, batch->count, lid_closed};
    display_policy_decide(&input, enabled);
}
