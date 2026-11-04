#include "cursor_sprite.h"

#include <math.h>
#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/xcursor.h>

struct cursor_sprite {
    struct wlr_cursor *cursor;
    struct wl_event_source *timer;
    struct wlr_xcursor *xcursor;
    size_t frame;
    int size;
};

static int handle_timer(void *data);
static void show_frame(cursor_sprite *sprite, size_t frame);
static void schedule_next(cursor_sprite *sprite, const struct wlr_xcursor_image *image);

cursor_sprite *cursor_sprite_create(struct wlr_cursor *cursor, struct wl_event_loop *loop)
{
    cursor_sprite *sprite = calloc(1, sizeof(*sprite));
    if (sprite == NULL) {
        return NULL;
    }
    sprite->cursor = cursor;
    sprite->timer = wl_event_loop_add_timer(loop, handle_timer, sprite);
    if (sprite->timer == NULL) {
        free(sprite);
        return NULL;
    }
    return sprite;
}

void cursor_sprite_destroy(cursor_sprite *sprite)
{
    wl_event_source_remove(sprite->timer);
    free(sprite);
}

bool cursor_sprite_show(cursor_sprite *sprite, const cursor_theme_view *theme, const char *name)
{
    if (theme->manager == NULL || !wlr_xcursor_manager_load(theme->manager, theme->scale)) {
        return false;
    }
    struct wlr_xcursor *xcursor = wlr_xcursor_manager_get_xcursor(theme->manager, name,
                                                                  theme->scale);
    if (xcursor == NULL || xcursor->image_count == 0) {
        return false;
    }
    sprite->xcursor = xcursor;
    sprite->size = theme->size;
    show_frame(sprite, 0);
    return true;
}

void cursor_sprite_stop(cursor_sprite *sprite)
{
    sprite->xcursor = NULL;
    wl_event_source_timer_update(sprite->timer, 0);
}

static int handle_timer(void *data)
{
    cursor_sprite *sprite = data;
    if (sprite->xcursor != NULL) {
        show_frame(sprite, (sprite->frame + 1) % sprite->xcursor->image_count);
    }
    return 0;
}

static void show_frame(cursor_sprite *sprite, size_t frame)
{
    struct wlr_xcursor_image *image = sprite->xcursor->images[frame];
    float scale = (float) image->width / (float) sprite->size;
    sprite->frame = frame;
    wlr_cursor_set_buffer(sprite->cursor, wlr_xcursor_image_get_buffer(image),
                          (int32_t) lroundf((float) image->hotspot_x / scale),
                          (int32_t) lroundf((float) image->hotspot_y / scale), scale);
    schedule_next(sprite, image);
}

static void schedule_next(cursor_sprite *sprite, const struct wlr_xcursor_image *image)
{
    bool animated = sprite->xcursor->image_count > 1 && image->delay > 0;
    wl_event_source_timer_update(sprite->timer, animated ? (int) image->delay : 0);
}
