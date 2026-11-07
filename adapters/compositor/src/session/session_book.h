#ifndef COMPOSITOR_SESSION_BOOK_H
#define COMPOSITOR_SESSION_BOOK_H

#include <stdbool.h>
#include <stddef.h>

#include "session_record.h"

enum {
    SESSION_BOOK_MAX = 256,
};

typedef struct session_book {
    session_record *records;
    size_t count;
    size_t capacity;
} session_book;

session_record *session_book_find(const session_book *book, const char *name);
bool session_book_put(session_book *book, const session_record *record);
bool session_book_remove(session_book *book, const char *name);
bool session_book_rename(session_book *book, const char *from, const char *to);
void session_book_trim(session_book *book, size_t max);
void session_book_finish(session_book *book);

#endif
