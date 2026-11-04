#ifndef XFCE_HELPER_CLIENT_H
#define XFCE_HELPER_CLIENT_H

#include <stdbool.h>

typedef struct helper_events {
    void *self;
    void (*line)(void *self, const char *line);
    void (*stopped)(void *self);
} helper_events;

typedef struct helper_client helper_client;

helper_client *helper_client_create(const helper_events *events);
void helper_client_destroy(helper_client *client);
void helper_client_start(helper_client *client);
bool helper_client_is_running(const helper_client *client);
void helper_client_send(helper_client *client, const char *line);
void helper_client_reset_backoff(helper_client *client);

#endif
