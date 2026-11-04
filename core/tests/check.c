#include "check.h"

#include <stdlib.h>

int check_failures;

int check_run(const check_case *cases, size_t count)
{
    size_t failed_cases = 0;

    for (size_t i = 0; i < count; i++) {
        int before = check_failures;
        cases[i].run();
        if (check_failures != before) {
            fprintf(stderr, "FAIL %s\n", cases[i].name);
            failed_cases++;
        }
    }
    fprintf(stderr, "%zu/%zu passed\n", count - failed_cases, count);
    return failed_cases == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
