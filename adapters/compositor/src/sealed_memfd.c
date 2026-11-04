#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wlr/util/log.h>

#include "sealed_memfd.h"

static const unsigned SEALS = F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_SEAL;

static bool write_all(int fd, const unsigned char *data, size_t length);

int sealed_memfd_create(const char *name, const void *data, size_t size)
{
    int fd = memfd_create(name, MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0) {
        wlr_log_errno(WLR_ERROR, "cannot create the %s memory file", name);
        return -1;
    }
    if (!write_all(fd, data, size) || fcntl(fd, F_ADD_SEALS, SEALS) != 0) {
        wlr_log_errno(WLR_ERROR, "cannot fill the %s memory file", name);
        close(fd);
        return -1;
    }
    return fd;
}

static bool write_all(int fd, const unsigned char *data, size_t length)
{
    while (length > 0) {
        ssize_t written = write(fd, data, length);
        if (written <= 0) {
            return false;
        }
        data += written;
        length -= (size_t) written;
    }
    return true;
}
