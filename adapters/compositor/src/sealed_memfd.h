#ifndef COMPOSITOR_SEALED_MEMFD_H
#define COMPOSITOR_SEALED_MEMFD_H

#include <stddef.h>

int sealed_memfd_create(const char *name, const void *data, size_t size);

#endif
