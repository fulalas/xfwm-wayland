#include "check.h"

#include <stdlib.h>

static int failures;

void check_fail(void)
{
    failures++;
}

int check_run(const check_case *cases, size_t count)
{
    size_t failed_cases = 0;

    for (size_t i = 0; i < count; i++) {
        int before = failures;
        cases[i].run();
        if (failures != before) {
            fprintf(stderr, "FAIL %s\n", cases[i].name);
            failed_cases++;
        }
    }
    fprintf(stderr, "%zu/%zu passed\n", count - failed_cases, count);
    return failed_cases == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
