#ifndef COMPOSITOR_PAGE_RUNS_H
#define COMPOSITOR_PAGE_RUNS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct page_run {
    size_t first;
    size_t count;
} page_run;

bool page_runs_next(const uint64_t *pagemap_entries, size_t entry_count, page_run *run);
size_t page_runs_file_pages(const uint64_t *pagemap_entries, size_t entry_count);

#endif
