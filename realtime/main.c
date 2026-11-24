#include <limits.h>
#include <poll.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
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

static int is_untraced(pid_t pid)
{
    char path[64];
    char line[256];
    long tracer = -1;
    snprintf(path, sizeof(path), "/proc/%ld/status", (long) pid);
    FILE *file = fopen(path, "r");
    if (!file) {
        return 0;
    }
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "TracerPid: %ld", &tracer) == 1) {
            break;
        }
    }
    fclose(file);
    return tracer == 0;
}

static int is_alive(int pidfd)
{
    struct pollfd watch = {.fd = pidfd, .events = POLLIN};
    return poll(&watch, 1, 0) == 0;
}

static int may_grant(pid_t parent, int pidfd)
{
    return getppid() == parent && is_compositor(parent) && is_untraced(parent) && is_alive(pidfd);
}

int main(void)
{
    pid_t parent = getppid();
    int pidfd = (int) syscall(SYS_pidfd_open, parent, 0);
    if (pidfd < 0) {
        return EXIT_FAILURE;
    }
    struct sched_param param = {.sched_priority = sched_get_priority_min(SCHED_RR)};
    int policy = SCHED_RR | SCHED_RESET_ON_FORK;
    int status = may_grant(parent, pidfd) && sched_setscheduler(parent, policy, &param) == 0
                     ? EXIT_SUCCESS
                     : EXIT_FAILURE;
    close(pidfd);
    return status;
}
