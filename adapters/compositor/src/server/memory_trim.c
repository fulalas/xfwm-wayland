#include "memory_trim.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-server-core.h>

#include "mapping_line.h"
#include "page_runs.h"
#include "util/clock.h"

enum {
    TICK_MS = 2000,
    ROUTINE_INTERVAL_MS = 60000,
    CALM_FRAMES_MAX = 10,
    GROWTH_BYTES = 16 * 1024 * 1024,
    MAPS_LINE_MAX = 512,
    PAGEMAP_CHUNK = 512,
};

struct memory_trim {
    struct wl_event_source *timer;
    bool armed;
    unsigned frames;
    bool settling;
    int64_t released_ms;
    size_t file_pages_kept;
};

static void arm(memory_trim *trim);
static int handle_timer(void *data);
static bool release_is_due(const memory_trim *trim);
static size_t resident_file_pages(void);
static size_t page_size(void);
static void release_pages(memory_trim *trim);
static void release_mappings(int pagemap);
static void release_file_pages(const mapping *mapping, int pagemap);
static void release_runs(uintptr_t base, const uint64_t *entries, size_t count);

memory_trim *memory_trim_create(struct wl_event_loop *loop)
{
    memory_trim *trim = calloc(1, sizeof(*trim));
    if (trim == NULL) {
        return NULL;
    }
    trim->released_ms = clock_now_ms() - ROUTINE_INTERVAL_MS;
    trim->timer = wl_event_loop_add_timer(loop, handle_timer, trim);
    if (trim->timer == NULL) {
        free(trim);
        return NULL;
    }
    return trim;
}

void memory_trim_destroy(memory_trim *trim)
{
    wl_event_source_remove(trim->timer);
    free(trim);
}

void memory_trim_note_frame(memory_trim *trim)
{
    if (trim == NULL) {
        return;
    }
    trim->frames++;
    if (!trim->armed) {
        arm(trim);
    }
}

static void arm(memory_trim *trim)
{
    trim->armed = wl_event_source_timer_update(trim->timer, TICK_MS) == 0;
}

static int handle_timer(void *data)
{
    memory_trim *trim = data;
    bool calm = trim->frames <= CALM_FRAMES_MAX;
    trim->armed = false;
    trim->frames = 0;
    if (!calm) {
        arm(trim);
    } else if (trim->settling) {
        trim->settling = false;
        trim->file_pages_kept = resident_file_pages();
    } else if (release_is_due(trim)) {
        release_pages(trim);
        arm(trim);
    }
    return 0;
}

static bool release_is_due(const memory_trim *trim)
{
    if (clock_now_ms() - trim->released_ms >= ROUTINE_INTERVAL_MS) {
        return true;
    }
    return resident_file_pages() >= trim->file_pages_kept + GROWTH_BYTES / page_size();
}

static size_t page_size(void)
{
    return (size_t) sysconf(_SC_PAGESIZE);
}

static size_t resident_file_pages(void)
{
    size_t pages = 0;
    FILE *statm = fopen("/proc/self/statm", "re");
    if (statm == NULL) {
        return 0;
    }
    if (fscanf(statm, "%*s %*s %zu", &pages) != 1) {
        pages = 0;
    }
    fclose(statm);
    return pages;
}

static void release_pages(memory_trim *trim)
{
    int pagemap = open("/proc/self/pagemap", O_RDONLY | O_CLOEXEC);
    if (pagemap >= 0) {
        release_mappings(pagemap);
        close(pagemap);
    }
    trim->settling = true;
    trim->released_ms = clock_now_ms();
}

static void release_mappings(int pagemap)
{
    FILE *maps = fopen("/proc/self/maps", "re");
    if (maps == NULL) {
        return;
    }
    char line[MAPS_LINE_MAX];
    bool at_line_start = true;
    while (fgets(line, sizeof(line), maps) != NULL) {
        mapping mapping;
        if (at_line_start && mapping_line_parse(line, &mapping)) {
            release_file_pages(&mapping, pagemap);
        }
        at_line_start = strchr(line, '\n') != NULL;
    }
    fclose(maps);
}

static void release_file_pages(const mapping *mapping, int pagemap)
{
    uint64_t entries[PAGEMAP_CHUNK];
    size_t page = page_size();
    size_t pages = mapping->length / page;
    for (size_t done = 0; done < pages; done += PAGEMAP_CHUNK) {
        size_t count = pages - done < PAGEMAP_CHUNK ? pages - done : PAGEMAP_CHUNK;
        uintptr_t base = mapping->start + done * page;
        off_t offset = (off_t) (base / page * sizeof(entries[0]));
        ssize_t got = pread(pagemap, entries, count * sizeof(entries[0]), offset);
        if (got <= 0) {
            return;
        }
        release_runs(base, entries, (size_t) got / sizeof(entries[0]));
    }
}

static void release_runs(uintptr_t base, const uint64_t *entries, size_t count)
{
    size_t page = page_size();
    page_run run = {0};
    while (page_runs_next(entries, count, &run)) {
        madvise((void *) (base + run.first * page), run.count * page, MADV_DONTNEED);
    }
}
