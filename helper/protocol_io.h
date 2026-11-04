#ifndef HELPER_PROTOCOL_IO_H
#define HELPER_PROTOCOL_IO_H

typedef struct icon_query {
    unsigned id;
    int size;
    const char *app_id;
    const char *icon_name;
} icon_query;

typedef struct request_handler {
    void *self;
    void (*icon)(void *self, const icon_query *query);
    void (*closed)(void *self);
} request_handler;

void protocol_listen(const request_handler *handler);
void protocol_write(const char *verb, const char *first, const char *second);

#endif
