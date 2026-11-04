#ifndef COMPOSITOR_CURSOR_SPRITE_H
#define COMPOSITOR_CURSOR_SPRITE_H

#include <stdbool.h>
#include <wayland-server-core.h>

struct wlr_cursor;
struct wlr_xcursor_manager;

typedef struct cursor_sprite cursor_sprite;

typedef struct cursor_theme_view {
    struct wlr_xcursor_manager *manager;
    float scale;
    int size;
} cursor_theme_view;

cursor_sprite *cursor_sprite_create(struct wlr_cursor *cursor, struct wl_event_loop *loop);
void cursor_sprite_destroy(cursor_sprite *sprite);
bool cursor_sprite_show(cursor_sprite *sprite, const cursor_theme_view *theme, const char *name);
void cursor_sprite_stop(cursor_sprite *sprite);

#endif
