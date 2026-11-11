#ifndef COMPOSITOR_MEMORY_TRIM_H
#define COMPOSITOR_MEMORY_TRIM_H

struct wl_event_loop;

typedef struct memory_trim memory_trim;

memory_trim *memory_trim_create(struct wl_event_loop *loop);
void memory_trim_destroy(memory_trim *trim);
void memory_trim_note_frame(memory_trim *trim);

#endif
