#ifndef CHECK_H
#define CHECK_H

#include <math.h>
#include <stdio.h>

typedef void (*check_test_fn)(void);

typedef struct check_case {
    const char *name;
    check_test_fn run;
} check_case;

void check_fail(void);

#define CHECK(condition)                                                                   \
    do {                                                                                   \
        if (!(condition)) {                                                                \
            fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition);  \
            check_fail();                                                                  \
        }                                                                                  \
    } while (0)

#define CHECK_INT_EQ(expected, actual)                                                     \
    do {                                                                                   \
        long check_e = (long) (expected);                                                  \
        long check_a = (long) (actual);                                                    \
        if (check_e != check_a) {                                                          \
            fprintf(stderr, "%s:%d: expected %s == %ld, got %ld\n", __FILE__, __LINE__,    \
                    #actual, check_e, check_a);                                            \
            check_fail();                                                                  \
        }                                                                                  \
    } while (0)

#define CHECK_RECT_EQ(expected, actual)                                                    \
    do {                                                                                   \
        wm_rect check_e = (expected);                                                      \
        wm_rect check_a = (actual);                                                        \
        if (!wm_rect_equals(&check_e, &check_a)) {                                         \
            fprintf(stderr, "%s:%d: expected %s == {%d,%d,%d,%d}, got {%d,%d,%d,%d}\n",   \
                    __FILE__, __LINE__, #actual, check_e.x, check_e.y, check_e.width,      \
                    check_e.height, check_a.x, check_a.y, check_a.width, check_a.height);  \
            check_fail();                                                                  \
        }                                                                                  \
    } while (0)

#define CHECK_DOUBLE_NEAR(expected, actual, tolerance)                                     \
    CHECK(fabs((double) (expected) - (double) (actual)) <= (tolerance))

int check_run(const check_case *cases, size_t count);

#endif
