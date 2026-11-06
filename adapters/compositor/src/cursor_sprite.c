#include "cursor_sprite.h"

#include <math.h>
#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/xcursor.h>

struct cursor_sprite {
    struct wlr_cursor *cursor;
    struct wl_event_source *timer;
    cursor_animation *animation;
    struct wlr_xcursor *xcursor;
    size_t frame;
    float scale;
};

static int handle_timer(void *data);
static size_t frame_count(const cursor_sprite *sprite);
static bool show_scalable(cursor_sprite *sprite, const cursor_theme_view *theme, const char *name);
static bool show_xcursor(cursor_sprite *sprite, const cursor_theme_view *theme, const char *name);
static void show_frame(cursor_sprite *sprite, size_t frame);
static void show_animation_frame(cursor_sprite *sprite, size_t frame);
static void show_xcursor_frame(cursor_sprite *sprite, size_t frame);
static void schedule_next(cursor_sprite *sprite, uint32_t delay_ms);

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
    return show_scalable(sprite, theme, name) || show_xcursor(sprite, theme, name);
}

void cursor_sprite_stop(cursor_sprite *sprite)
{
    sprite->animation = NULL;
    sprite->xcursor = NULL;
    wl_event_source_timer_update(sprite->timer, 0);
}

static int handle_timer(void *data)
{
    cursor_sprite *sprite = data;
    size_t count = frame_count(sprite);
    if (count > 0) {
        show_frame(sprite, (sprite->frame + 1) % count);
    }
    return 0;
}

static size_t frame_count(const cursor_sprite *sprite)
{
    if (sprite->animation != NULL) {
        return cursor_animation_length(sprite->animation);
    }
    return sprite->xcursor != NULL ? sprite->xcursor->image_count : 0;
}

static bool show_scalable(cursor_sprite *sprite, const cursor_theme_view *theme, const char *name)
{
    cursor_animation *animation =
        theme->scalable != NULL ? scalable_theme_cursor(theme->scalable, name) : NULL;
    if (animation == NULL) {
        return false;
    }
    sprite->animation = animation;
    sprite->xcursor = NULL;
    show_frame(sprite, 0);
    return true;
}

static bool show_xcursor(cursor_sprite *sprite, const cursor_theme_view *theme, const char *name)
{
    if (theme->manager == NULL || !wlr_xcursor_manager_load(theme->manager, theme->scale)) {
        return false;
    }
    struct wlr_xcursor *xcursor = wlr_xcursor_manager_get_xcursor(theme->manager, name,
                                                                  theme->scale);
    if (xcursor == NULL || xcursor->image_count == 0) {
        return false;
    }
    sprite->animation = NULL;
    sprite->xcursor = xcursor;
    sprite->scale = theme->scale > 0.0f ? theme->scale : 1.0f;
    show_frame(sprite, 0);
    return true;
}

static void show_frame(cursor_sprite *sprite, size_t frame)
{
    sprite->frame = frame;
    if (sprite->animation != NULL) {
        show_animation_frame(sprite, frame);
    } else {
        show_xcursor_frame(sprite, frame);
    }
}

static void show_animation_frame(cursor_sprite *sprite, size_t frame)
{
    const cursor_frame *current = cursor_animation_frame(sprite->animation, frame);
    if (current == NULL) {
        cursor_sprite_stop(sprite);
        return;
    }
    wlr_cursor_set_buffer(sprite->cursor, current->buffer, current->hotspot_x,
                          current->hotspot_y, current->scale);
    schedule_next(sprite, current->delay_ms);
}

static void show_xcursor_frame(cursor_sprite *sprite, size_t frame)
{
    struct wlr_xcursor_image *image = sprite->xcursor->images[frame];
    wlr_cursor_set_buffer(sprite->cursor, wlr_xcursor_image_get_buffer(image),
                          (int32_t) lroundf((float) image->hotspot_x / sprite->scale),
                          (int32_t) lroundf((float) image->hotspot_y / sprite->scale),
                          sprite->scale);
    schedule_next(sprite, image->delay);
}

static void schedule_next(cursor_sprite *sprite, uint32_t delay_ms)
{
    bool animated = frame_count(sprite) > 1 && delay_ms > 0;
    wl_event_source_timer_update(sprite->timer, animated ? (int) delay_ms : 0);
}
