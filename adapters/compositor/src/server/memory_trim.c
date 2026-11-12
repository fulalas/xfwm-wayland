#include "memory_trim.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-server-core.h>

#include "mapping_line.h"
#include "mapping_set.h"
#include "page_runs.h"
#include "trim_rule.h"
#include "util/clock.h"

enum {
    RELEASED_SET_MIN_BYTES = 1024 * 1024,
    MAPS_LINE_MAX = 512,
    PAGEMAP_CHUNK = 512,
};

typedef enum scan_mode {
    SCAN_RELEASE,
    SCAN_COUNT_NEW,
} scan_mode;

struct memory_trim {
    struct wl_event_source *timer;
    int64_t due_ms;
    unsigned frames;
    trim_state state;
    mapping_set released;
};

static void arm(memory_trim *trim, int delay_ms);
static int handle_timer(void *data);
static trim_plan check_new(memory_trim *trim, const trim_tick *tick);
static size_t resident_file_bytes(void);
static void release_pages(memory_trim *trim, int64_t now);
static size_t scan_mappings(memory_trim *trim, scan_mode mode);
static size_t scan_mapping(memory_trim *trim, const mapping *mapping, int pagemap,
                           scan_mode mode);
static size_t scan_file_pages(const mapping *mapping, int pagemap, scan_mode mode);
static void release_runs(uintptr_t base, const uint64_t *entries, size_t count);
static size_t page_size(void);

memory_trim *memory_trim_create(struct wl_event_loop *loop)
{
    memory_trim *trim = calloc(1, sizeof(*trim));
    if (trim == NULL) {
        return NULL;
    }
    trim->state = trim_rule_start(clock_now_ms());
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
    if (trim->due_ms == 0 || trim->due_ms - clock_now_ms() > TRIM_TICK_MS) {
        arm(trim, TRIM_TICK_MS);
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
    trim_tick tick = {clock_now_ms(), resident_file_bytes(), trim->frames};
    trim->due_ms = 0;
    trim->frames = 0;
    trim_plan plan = trim_rule_plan(&trim->state, &tick);
    if (plan.step == TRIM_CHECK_NEW) {
        plan = check_new(trim, &tick);
    }
    trim->state.backoff_ms = plan.backoff_ms;
    if (plan.step == TRIM_RELEASE) {
        release_pages(trim, tick.now_ms);
    } else if (plan.step == TRIM_WAIT) {
        arm(trim, plan.wait_ms);
    }
    return 0;
}

static trim_plan check_new(memory_trim *trim, const trim_tick *tick)
{
    if (scan_mappings(trim, SCAN_COUNT_NEW) >= TRIM_GROWTH_BYTES) {
        return (trim_plan) {.step = TRIM_RELEASE, .backoff_ms = TRIM_BACKOFF_MIN_MS};
    }
    trim->state.bytes_checked = tick->bytes;
    return trim_rule_plan(&trim->state, tick);
}

static size_t resident_file_bytes(void)
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
    return pages * page_size();
}

static void release_pages(memory_trim *trim, int64_t now)
{
    trim->state.bytes_before = resident_file_bytes();
    mapping_set_clear(&trim->released);
    scan_mappings(trim, SCAN_RELEASE);
    trim->state.bytes_after = resident_file_bytes();
    trim->state.bytes_checked = trim->state.bytes_after;
    trim->state.released_ms = now;
}

static size_t scan_mappings(memory_trim *trim, scan_mode mode)
{
    int pagemap = open("/proc/self/pagemap", O_RDONLY | O_CLOEXEC);
    FILE *maps = pagemap >= 0 ? fopen("/proc/self/maps", "re") : NULL;
    size_t new_bytes = 0;
    char line[MAPS_LINE_MAX];
    bool at_line_start = true;
    while (maps != NULL && fgets(line, sizeof(line), maps) != NULL) {
        mapping mapping;
        if (at_line_start && mapping_line_parse(line, &mapping)) {
            new_bytes += scan_mapping(trim, &mapping, pagemap, mode);
        }
        at_line_start = strchr(line, '\n') != NULL;
    }
    if (maps != NULL) {
        fclose(maps);
    }
    if (pagemap >= 0) {
        close(pagemap);
    }
    return new_bytes;
}

static size_t scan_mapping(memory_trim *trim, const mapping *mapping, int pagemap,
                           scan_mode mode)
{
    if (mode == SCAN_COUNT_NEW) {
        return mapping_set_has(&trim->released, mapping)
                   ? 0
                   : scan_file_pages(mapping, pagemap, mode) * page_size();
    }
    if (scan_file_pages(mapping, pagemap, mode) * page_size() >= RELEASED_SET_MIN_BYTES) {
        mapping_set_add(&trim->released, mapping);
    }
    return 0;
}

static size_t scan_file_pages(const mapping *mapping, int pagemap, scan_mode mode)
{
    uint64_t entries[PAGEMAP_CHUNK];
    size_t page = page_size();
    size_t pages = mapping->length / page;
    size_t file_pages = 0;
    for (size_t done = 0; done < pages; done += PAGEMAP_CHUNK) {
        size_t count = pages - done < PAGEMAP_CHUNK ? pages - done : PAGEMAP_CHUNK;
        uintptr_t base = mapping->start + done * page;
        off_t offset = (off_t) (base / page * sizeof(entries[0]));
        ssize_t got = pread(pagemap, entries, count * sizeof(entries[0]), offset);
        if (got <= 0) {
            break;
        }
        size_t read = (size_t) got / sizeof(entries[0]);
        file_pages += page_runs_file_pages(entries, read);
        if (mode == SCAN_RELEASE) {
            release_runs(base, entries, read);
        }
    }
    return file_pages;
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
