#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_scene.h>

#include "render/scene_buffers.h"
#include "shadow.h"
#include "wm/shadow.h"

struct shadow {
    struct wlr_scene_tree *tree;
    struct wlr_scene_buffer *parts[SHADOW_PART_COUNT];
    struct wlr_buffer *image;
};

static void use_image(shadow *shadow, struct wlr_buffer *image);
static void place_part(struct wlr_scene_buffer *part, const shadow_piece *piece);

shadow *shadow_create(struct wlr_scene_tree *parent)
{
    shadow *s = calloc(1, sizeof(*s));
    if (s == NULL) {
        return NULL;
    }
    s->tree = wlr_scene_tree_create(parent);
    if (s->tree == NULL) {
        free(s);
        return NULL;
    }
    for (int i = 0; i < SHADOW_PART_COUNT; i++) {
        s->parts[i] = wlr_scene_buffer_create(s->tree, NULL);
        if (s->parts[i] != NULL) {
            s->parts[i]->point_accepts_input = scene_buffer_reject_input;
        }
    }
    wlr_scene_node_set_enabled(&s->tree->node, false);
    return s;
}

void shadow_destroy(shadow *s)
{
    use_image(s, NULL);
    wlr_scene_node_destroy(&s->tree->node);
    free(s);
}

void shadow_release(shadow *s)
{
    if (s->image != NULL) {
        wlr_buffer_unlock(s->image);
    }
    free(s);
}

void shadow_show(shadow *s, const shadow_image *image, const wm_rect *rect)
{
    if (image->buffer == NULL) {
        shadow_hide(s);
        return;
    }
    use_image(s, image->buffer);
    shadow_pieces pieces = shadow_layout_compute(rect, &image->geometry);
    for (int i = 0; i < SHADOW_PART_COUNT; i++) {
        place_part(s->parts[i], &pieces.pieces[i]);
    }
    wlr_scene_node_set_enabled(&s->tree->node, true);
}

void shadow_show_around(shadow *s, compositor *server, const wm_rect *frame)
{
    const wm_compositing_config *config = &server->config.compositing;
    wm_rect rect = wm_shadow_rect(frame, config);
    shadow_image image = shadow_cache_get(server->shadows);
    shadow_show(s, &image, &rect);
    shadow_set_opacity(s, (float) scene_opacity_from_percent(config->shadow_opacity_percent));
}

void shadow_hide(shadow *s)
{
    wlr_scene_node_set_enabled(&s->tree->node, false);
}

void shadow_set_opacity(shadow *s, float opacity)
{
    for (int i = 0; i < SHADOW_PART_COUNT; i++) {
        if (s->parts[i] != NULL) {
            wlr_scene_buffer_set_opacity(s->parts[i], opacity);
        }
    }
}

static void use_image(shadow *s, struct wlr_buffer *image)
{
    if (s->image == image) {
        return;
    }
    if (s->image != NULL) {
        wlr_buffer_unlock(s->image);
    }
    s->image = image != NULL ? wlr_buffer_lock(image) : NULL;
    for (int i = 0; i < SHADOW_PART_COUNT; i++) {
        if (s->parts[i] != NULL) {
            wlr_scene_buffer_set_buffer(s->parts[i], image);
        }
    }
}

static void place_part(struct wlr_scene_buffer *part, const shadow_piece *piece)
{
    if (part == NULL) {
        return;
    }
    bool visible = piece->dest.width > 0 && piece->dest.height > 0 &&
                   piece->source.width > 0 && piece->source.height > 0;
    wlr_scene_node_set_enabled(&part->node, visible);
    if (!visible) {
        return;
    }
    struct wlr_fbox source = {piece->source.x, piece->source.y, piece->source.width,
                              piece->source.height};
    wlr_scene_buffer_set_source_box(part, &source);
    wlr_scene_buffer_set_dest_size(part, piece->dest.width, piece->dest.height);
    wlr_scene_node_set_position(&part->node, piece->dest.x, piece->dest.y);
}
