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
#include "wm/geometry.h"

enum {
    TICK_MS = 500,
    BACKOFF_MIN_MS = 2000,
    BACKOFF_MAX_MS = 64000,
    ROUTINE_INTERVAL_MS = 60000,
    CALM_FRAMES_MAX = 2,
    GROWTH_BYTES = 16 * 1024 * 1024,
    LEFTOVER_BYTES = 1024 * 1024,
    MAPS_LINE_MAX = 512,
    PAGEMAP_CHUNK = 512,
};

struct memory_trim {
    struct wl_event_source *timer;
    int64_t due_ms;
    unsigned frames;
    int backoff_ms;
    int64_t released_ms;
    size_t pages_before;
    size_t pages_after;
};

static void arm(memory_trim *trim, int delay_ms);
static int handle_timer(void *data);
static void schedule(memory_trim *trim, bool calm, int64_t since, size_t pages);
static bool above(size_t pages, size_t base, size_t bytes);
static size_t resident_file_pages(void);
static void release_pages(memory_trim *trim, int64_t now, int backoff_ms);
static void release_mappings(int pagemap);
static void release_file_pages(const mapping *mapping, int pagemap);
static void release_runs(uintptr_t base, const uint64_t *entries, size_t count);
static size_t page_size(void);

memory_trim *memory_trim_create(struct wl_event_loop *loop)
{
    memory_trim *trim = calloc(1, sizeof(*trim));
    if (trim == NULL) {
        return NULL;
    }
    trim->backoff_ms = BACKOFF_MIN_MS;
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
    if (trim->due_ms == 0 || trim->due_ms - clock_now_ms() > TICK_MS) {
        arm(trim, TICK_MS);
    }
}

static void arm(memory_trim *trim, int delay_ms)
{
    bool armed = wl_event_source_timer_update(trim->timer, delay_ms) == 0;
    trim->due_ms = armed ? clock_now_ms() + delay_ms : 0;
}

static int handle_timer(void *data)
{
    memory_trim *trim = data;
    bool calm = trim->frames <= CALM_FRAMES_MAX;
    int64_t now = clock_now_ms();
    int64_t since = now - trim->released_ms;
    size_t pages = resident_file_pages();
    trim->due_ms = 0;
    trim->frames = 0;
    if (above(pages, trim->pages_before, GROWTH_BYTES)) {
        release_pages(trim, now, BACKOFF_MIN_MS);
    } else if (since >= trim->backoff_ms && above(pages, trim->pages_after, GROWTH_BYTES)) {
        release_pages(trim, now, wm_min(trim->backoff_ms * 2, BACKOFF_MAX_MS));
    } else if (calm && since >= ROUTINE_INTERVAL_MS &&
               above(pages, trim->pages_after, LEFTOVER_BYTES)) {
        release_pages(trim, now, BACKOFF_MIN_MS);
    } else {
        schedule(trim, calm, since, pages);
    }
    return 0;
}

static void schedule(memory_trim *trim, bool calm, int64_t since, size_t pages)
{
    if (since >= trim->backoff_ms) {
        trim->backoff_ms = BACKOFF_MIN_MS;
    }
    if (!calm) {
        arm(trim, TICK_MS);
    } else if (above(pages, trim->pages_after, GROWTH_BYTES)) {
        arm(trim, wm_max((int) (trim->backoff_ms - since), TICK_MS));
    } else if (above(pages, trim->pages_after, LEFTOVER_BYTES)) {
        arm(trim, wm_max((int) (ROUTINE_INTERVAL_MS - since), TICK_MS));
    }
}

static bool above(size_t pages, size_t base, size_t bytes)
{
    return pages >= base + bytes / page_size();
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

static void release_pages(memory_trim *trim, int64_t now, int backoff_ms)
{
    int pagemap = open("/proc/self/pagemap", O_RDONLY | O_CLOEXEC);
    trim->pages_before = resident_file_pages();
    if (pagemap >= 0) {
        release_mappings(pagemap);
        close(pagemap);
    }
    trim->pages_after = resident_file_pages();
    trim->released_ms = now;
    trim->backoff_ms = backoff_ms;
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

static size_t page_size(void)
{
    return (size_t) sysconf(_SC_PAGESIZE);
}
