#ifndef COMPOSITOR_SESSION_FILE_H
#define COMPOSITOR_SESSION_FILE_H

#include <stdbool.h>

#include "session_book.h"

char *session_file_new_id(void);
bool session_file_load(const char *id, session_book *book);
bool session_file_save(const char *id, const session_book *book);
void session_file_remove(const char *id);
void session_file_evict_expired(void);

#endif
