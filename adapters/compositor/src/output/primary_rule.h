#ifndef COMPOSITOR_PRIMARY_RULE_H
#define COMPOSITOR_PRIMARY_RULE_H

#include <stdbool.h>

typedef enum primary_event {
    PRIMARY_EVENT_NONE,
    PRIMARY_EVENT_GAINED,
    PRIMARY_EVENT_LOST,
} primary_event;

primary_event primary_rule_event(bool sent_primary, bool is_primary, bool client_hears_loss);

#endif
