#ifndef XFCE_DISPATCHER_H
#define XFCE_DISPATCHER_H

typedef struct dispatch_target {
    void *self;
    void (*run)(void *self, unsigned flags);
} dispatch_target;

typedef struct dispatcher dispatcher;

dispatcher *dispatcher_create(const dispatch_target *target);
void dispatcher_destroy(dispatcher *dispatcher);
void dispatcher_post(dispatcher *dispatcher, unsigned flags);

#endif
