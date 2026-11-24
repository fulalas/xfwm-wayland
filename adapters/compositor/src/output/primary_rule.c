#include "primary_rule.h"

primary_event primary_rule_event(bool sent_primary, bool is_primary, bool client_hears_loss)
{
    if (is_primary && !sent_primary) {
        return PRIMARY_EVENT_GAINED;
    }
    if (!is_primary && sent_primary && client_hears_loss) {
        return PRIMARY_EVENT_LOST;
    }
    return PRIMARY_EVENT_NONE;
}
