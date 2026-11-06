#include "cursor_sprite.h"

#include <math.h>
#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/xcursor.h>

#include "cairo_buffer.h"
#include "scalable_layout.h"
#include "xcursor_resize.h"

struct cursor_sprite {
    struct wlr_cursor *cursor;
    struct wl_event_source *timer;
    cursor_animation *animation;
    struct wlr_xcursor *xcursor;
    size_t frame;
    float scale;
    int size;
    struct wlr_buffer **resized;
    size_t resized_count;
};

static int handle_timer(void *data);
static size_t frame_count(const cursor_sprite *sprite);
static bool show_scalable(cursor_sprite *sprite, const cursor_theme_view *theme, const char *name);
static bool show_xcursor(cursor_sprite *sprite, const cursor_theme_view *theme, const char *name);
static void show_frame(cursor_sprite *sprite, size_t frame);
static void show_animation_frame(cursor_sprite *sprite, size_t frame);
static void show_xcursor_frame(cursor_sprite *sprite, size_t frame);
static void schedule_next(cursor_sprite *sprite, uint32_t delay_ms);
static struct wlr_buffer *resized_frame(cursor_sprite *sprite, size_t frame,
                                        const scalable_layout *layout);
static struct wlr_buffer *resize_image(const struct wlr_xcursor_image *image,
                                       const scalable_layout *layout);
static void drop_resized(cursor_sprite *sprite);

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
    drop_resized(sprite);
    free(sprite);
}

bool cursor_sprite_show(cursor_sprite *sprite, const cursor_theme_view *theme, const char *name)
{
    return show_scalable(sprite, theme, name) || show_xcursor(sprite, theme, name);
}

void cursor_sprite_stop(cursor_sprite *sprite)
{
    drop_resized(sprite);
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
    float scale = theme->scale > 0.0f ? theme->scale : 1.0f;
    if (xcursor != sprite->xcursor || scale != sprite->scale || theme->size != sprite->size) {
        drop_resized(sprite);
    }
    sprite->animation = NULL;
    sprite->xcursor = xcursor;
    sprite->scale = scale;
    sprite->size = theme->size;
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
    struct wlr_buffer *buffer = NULL;
    scalable_layout layout = {0};
    if (image->width > 0 && image->height > 0) {
        scalable_request request = {image->width, image->width, sprite->size, sprite->scale};
        layout = scalable_layout_for(&request);
        buffer = layout.pixels == (int) image->width ? wlr_xcursor_image_get_buffer(image)
                                                     : resized_frame(sprite, frame, &layout);
    }
    if (buffer != NULL) {
        wlr_cursor_set_buffer(sprite->cursor, buffer,
                              (int32_t) lround(image->hotspot_x * layout.hotspot_factor),
                              (int32_t) lround(image->hotspot_y * layout.hotspot_factor),
                              layout.buffer_scale);
    } else {
        wlr_cursor_set_buffer(sprite->cursor, wlr_xcursor_image_get_buffer(image),
                              (int32_t) lroundf((float) image->hotspot_x / sprite->scale),
                              (int32_t) lroundf((float) image->hotspot_y / sprite->scale),
                              sprite->scale);
    }
    schedule_next(sprite, image->delay);
}

static struct wlr_buffer *resized_frame(cursor_sprite *sprite, size_t frame,
                                        const scalable_layout *layout)
{
    if (sprite->resized == NULL) {
        sprite->resized = calloc(sprite->xcursor->image_count, sizeof(*sprite->resized));
        sprite->resized_count = sprite->resized != NULL ? sprite->xcursor->image_count : 0;
    }
    if (frame >= sprite->resized_count) {
        return NULL;
    }
    if (sprite->resized[frame] == NULL) {
        sprite->resized[frame] = resize_image(sprite->xcursor->images[frame], layout);
    }
    return sprite->resized[frame];
}

static struct wlr_buffer *resize_image(const struct wlr_xcursor_image *image,
                                       const scalable_layout *layout)
{
    int width = layout->pixels;
    int height = xcursor_resized_length((int) image->height, layout->pixels, (int) image->width);
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS ||
        cairo_image_surface_get_stride(surface) != width * 4) {
        cairo_surface_destroy(surface);
        return NULL;
    }
    cairo_surface_flush(surface);
    xcursor_pixels source = {(const uint32_t *) image->buffer, (int) image->width,
                             (int) image->height};
    xcursor_resize(&source, (uint32_t *) cairo_image_surface_get_data(surface), width, height);
    cairo_surface_mark_dirty(surface);
    struct wlr_buffer *buffer = cairo_buffer_create(surface);
    cairo_surface_destroy(surface);
    return buffer;
}

static void drop_resized(cursor_sprite *sprite)
{
    for (size_t i = 0; i < sprite->resized_count; i++) {
        if (sprite->resized[i] != NULL) {
            wlr_buffer_drop(sprite->resized[i]);
        }
    }
    free(sprite->resized);
    sprite->resized = NULL;
    sprite->resized_count = 0;
}

static void schedule_next(cursor_sprite *sprite, uint32_t delay_ms)
{
    bool animated = frame_count(sprite) > 1 && delay_ms > 0;
    wl_event_source_timer_update(sprite->timer, animated ? (int) delay_ms : 0);
}
