#include <limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int is_compositor(pid_t pid)
{
    char link[64];
    char target[PATH_MAX];
    snprintf(link, sizeof(link), "/proc/%ld/exe", (long) pid);
    ssize_t length = readlink(link, target, sizeof(target) - 1);
    if (length < 0) {
        return 0;
    }
    target[length] = '\0';
    return strcmp(target, REALTIME_COMPOSITOR_PATH) == 0;
}

int main(void)
{
    pid_t parent = getppid();
    if (!is_compositor(parent)) {
        return EXIT_FAILURE;
    }
    struct sched_param param = {.sched_priority = sched_get_priority_min(SCHED_RR)};
    int policy = SCHED_RR | SCHED_RESET_ON_FORK;
    return sched_setscheduler(parent, policy, &param) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
