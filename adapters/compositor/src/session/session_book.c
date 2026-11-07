#include "session_book.h"

#include <stdlib.h>
#include <string.h>

static bool reserve(session_book *book);
static void remove_at(session_book *book, size_t index);
static size_t oldest_index(const session_book *book);

session_record *session_book_find(const session_book *book, const char *name)
{
    for (size_t i = 0; i < book->count; i++) {
        if (strcmp(book->records[i].name, name) == 0) {
            return &book->records[i];
        }
    }
    return NULL;
}

bool session_book_put(session_book *book, const session_record *record)
{
    session_record *existing = session_book_find(book, record->name);
    if (existing != NULL) {
        char *name = existing->name;
        *existing = *record;
        existing->name = name;
        return true;
    }
    char *name = strdup(record->name);
    if (name == NULL || !reserve(book)) {
        free(name);
        return false;
    }
    book->records[book->count] = *record;
    book->records[book->count].name = name;
    book->count++;
    return true;
}

bool session_book_remove(session_book *book, const char *name)
{
    session_record *record = session_book_find(book, name);
    if (record == NULL) {
        return false;
    }
    remove_at(book, (size_t) (record - book->records));
    return true;
}

bool session_book_rename(session_book *book, const char *from, const char *to)
{
    session_record *record = session_book_find(book, from);
    char *name = record != NULL && session_book_find(book, to) == NULL ? strdup(to) : NULL;
    if (name == NULL) {
        return false;
    }
    free(record->name);
    record->name = name;
    return true;
}

void session_book_trim(session_book *book, size_t max)
{
    while (book->count > max) {
        remove_at(book, oldest_index(book));
    }
}

void session_book_finish(session_book *book)
{
    for (size_t i = 0; i < book->count; i++) {
        session_record_finish(&book->records[i]);
    }
    free(book->records);
    *book = (session_book){0};
}

static bool reserve(session_book *book)
{
    if (book->count < book->capacity) {
        return true;
    }
    size_t capacity = book->capacity == 0 ? 8 : book->capacity * 2;
    session_record *records = realloc(book->records, capacity * sizeof(*records));
    if (records == NULL) {
        return false;
    }
    book->records = records;
    book->capacity = capacity;
    return true;
}

static void remove_at(session_book *book, size_t index)
{
    session_record_finish(&book->records[index]);
    book->count--;
    memmove(&book->records[index], &book->records[index + 1],
            (book->count - index) * sizeof(book->records[0]));
}

static size_t oldest_index(const session_book *book)
{
    size_t oldest = 0;
    for (size_t i = 1; i < book->count; i++) {
        if (book->records[i].seen < book->records[oldest].seen) {
            oldest = i;
        }
    }
    return oldest;
}
