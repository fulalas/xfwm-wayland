#ifndef XFCE_TERMINATE_H
#define XFCE_TERMINATE_H

#include "compositor/services.h"

typedef struct terminate_dialogs terminate_dialogs;

terminate_dialogs *terminate_dialogs_create(void);
void terminate_dialogs_destroy(terminate_dialogs *dialogs);
void terminate_dialogs_ask(terminate_dialogs *dialogs, const terminate_request *request);
void terminate_dialogs_cancel(terminate_dialogs *dialogs, void *context);

#endif
