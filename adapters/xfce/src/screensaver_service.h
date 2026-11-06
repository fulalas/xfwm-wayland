#ifndef XFCE_SCREENSAVER_SERVICE_H
#define XFCE_SCREENSAVER_SERVICE_H

#include <stdbool.h>

typedef struct screensaver_service screensaver_service;

typedef struct screensaver_listener {
    void *self;
    void (*inhibit_changed)(void *self);
    void (*activity)(void *self);
} screensaver_listener;

screensaver_service *screensaver_service_create(const screensaver_listener *listener);
void screensaver_service_destroy(screensaver_service *service);
bool screensaver_service_inhibited(const screensaver_service *service);

#endif
