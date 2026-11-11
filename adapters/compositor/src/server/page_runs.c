#include "page_runs.h"

#define PAGE_PRESENT (UINT64_C(1) << 63)
#define PAGE_SWAPPED (UINT64_C(1) << 62)
#define PAGE_FROM_FILE (UINT64_C(1) << 61)

static bool is_file_page(uint64_t entry);
static bool is_unmapped(uint64_t entry);

bool page_runs_next(const uint64_t *entries, size_t entry_count, page_run *run)
{
    size_t first = run->first + run->count;
    while (first < entry_count && !is_file_page(entries[first])) {
        first++;
    }
    size_t end = first;
    for (size_t at = first; at < entry_count; at++) {
        if (is_file_page(entries[at])) {
            end = at + 1;
        } else if (!is_unmapped(entries[at])) {
            break;
        }
    }
    run->first = first;
    run->count = end - first;
    return run->count > 0;
}

static bool is_file_page(uint64_t entry)
{
    return (entry & PAGE_PRESENT) != 0 && (entry & PAGE_FROM_FILE) != 0;
}

static bool is_unmapped(uint64_t entry)
{
    return (entry & (PAGE_PRESENT | PAGE_SWAPPED)) == 0;
}
