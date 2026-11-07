#include <string.h>

#include "check.h"
#include "session/session_book.h"

static session_record record(const char *name, long seen)
{
    return (session_record){.name = (char *) name, .geometry = {0, 0, 10, 10}, .seen = seen};
}

static void fill(session_book *book)
{
    session_record a = record("a", 30);
    session_record b = record("b", 10);
    session_record c = record("c", 20);
    session_book_put(book, &a);
    session_book_put(book, &b);
    session_book_put(book, &c);
}

static void test_put_replaces_record_with_same_name(void)
{
    session_book book = {0};
    fill(&book);
    session_record moved = record("b", 40);
    moved.geometry.x = 7;
    CHECK(session_book_put(&book, &moved));
    CHECK_INT_EQ(3, book.count);
    CHECK_INT_EQ(7, session_book_find(&book, "b")->geometry.x);
    session_book_finish(&book);
}

static void test_book_owns_its_names(void)
{
    session_book book = {0};
    char name[] = "window";
    session_record r = record(name, 1);
    session_book_put(&book, &r);
    name[0] = 'X';
    CHECK(session_book_find(&book, "window") != NULL);
    session_book_finish(&book);
}

static void test_remove_and_rename(void)
{
    session_book book = {0};
    fill(&book);
    CHECK(session_book_remove(&book, "a"));
    CHECK(!session_book_remove(&book, "a"));
    CHECK(!session_book_rename(&book, "b", "c"));
    CHECK(session_book_rename(&book, "b", "d"));
    CHECK(session_book_find(&book, "b") == NULL);
    CHECK(session_book_find(&book, "d") != NULL);
    CHECK_INT_EQ(2, book.count);
    session_book_finish(&book);
}

static void test_trim_drops_least_recently_seen(void)
{
    session_book book = {0};
    fill(&book);
    session_book_trim(&book, 2);
    CHECK_INT_EQ(2, book.count);
    CHECK(session_book_find(&book, "b") == NULL);
    CHECK(session_book_find(&book, "a") != NULL);
    CHECK(session_book_find(&book, "c") != NULL);
    session_book_finish(&book);
}

int main(void)
{
    static const check_case cases[] = {
        {"put_replaces_record_with_same_name", test_put_replaces_record_with_same_name},
        {"book_owns_its_names", test_book_owns_its_names},
        {"remove_and_rename", test_remove_and_rename},
        {"trim_drops_least_recently_seen", test_trim_drops_least_recently_seen},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
