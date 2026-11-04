#include <sched.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    struct sched_param param = {.sched_priority = sched_get_priority_min(SCHED_RR)};
    int policy = SCHED_RR | SCHED_RESET_ON_FORK;
    return sched_setscheduler(getppid(), policy, &param) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
