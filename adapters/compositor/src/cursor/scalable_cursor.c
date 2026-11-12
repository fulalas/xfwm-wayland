#include "scalable_cursor.h"

#include <cairo.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include <librsvg/rsvg.h>
#include <math.h>
#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/util/log.h>

#include "render/cairo_buffer.h"
#include "scalable_layout.h"
#include "xcursor_file.h"

static const char scalable_dir[] = "cursors_scalable";
static const char default_theme[] = "default";

struct scalable_theme {
    char *dir;
    int size;
    double scale;
    GHashTable *cursors;
};

typedef struct frame_entry {
    char *path;
    double nominal;
    double hotspot_x;
    double hotspot_y;
    uint32_t delay_ms;
} frame_entry;

struct cursor_animation {
    frame_entry *entries;
    cursor_frame *frames;
    size_t count;
    int size;
    double scale;
};

static char *find_theme_dir(const char *name);
static char *existing_dir(char *path);
static cursor_animation *load_cursor(const scalable_theme *theme, const char *name);
static cursor_animation *load_frames(const scalable_theme *theme, const char *cursor_dir,
                                     JsonArray *entries);
static bool read_entries(cursor_animation *animation, const char *cursor_dir,
                         JsonArray *entries);
static bool read_entry(JsonObject *object, const char *cursor_dir, frame_entry *entry);
static bool render_frame(cursor_animation *animation, size_t index);
static cairo_surface_t *render_svg(RsvgHandle *svg, int pixels);
static double canvas_size(RsvgHandle *svg, double nominal);
static void animation_free(gpointer data);

scalable_theme *scalable_theme_open(const scalable_theme_request *request)
{
    char *dir = find_theme_dir(request->name[0] != '\0' ? request->name : default_theme);
    if (dir == NULL) {
        return NULL;
    }
    scalable_theme *theme = g_new0(scalable_theme, 1);
    theme->dir = dir;
    theme->size = request->size;
    theme->scale = request->scale;
    theme->cursors = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, animation_free);
    return theme;
}

void scalable_theme_close(scalable_theme *theme)
{
    g_hash_table_destroy(theme->cursors);
    g_free(theme->dir);
    g_free(theme);
}

cursor_animation *scalable_theme_cursor(scalable_theme *theme, const char *name)
{
    gpointer cached = NULL;
    if (g_hash_table_lookup_extended(theme->cursors, name, NULL, &cached)) {
        return cached;
    }
    cursor_animation *animation = load_cursor(theme, name);
    g_hash_table_insert(theme->cursors, g_strdup(name), animation);
    return animation;
}

size_t cursor_animation_length(const cursor_animation *animation)
{
    return animation->count;
}

const cursor_frame *cursor_animation_frame(cursor_animation *animation, size_t index)
{
    if (animation->frames[index].buffer == NULL && !render_frame(animation, index)) {
        return NULL;
    }
    return &animation->frames[index];
}

static char *find_theme_dir(const char *name)
{
    char **path = xcursor_search_path();
    char *found = NULL;
    for (char **dir = path; found == NULL && *dir != NULL; dir++) {
        found = existing_dir(g_build_filename(*dir, name, scalable_dir, NULL));
    }
    g_strfreev(path);
    return found;
}

static char *existing_dir(char *path)
{
    if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        return path;
    }
    g_free(path);
    return NULL;
}

static cursor_animation *load_cursor(const scalable_theme *theme, const char *name)
{
    char *cursor_dir = g_build_filename(theme->dir, name, NULL);
    char *metadata = g_build_filename(cursor_dir, "metadata.json", NULL);
    JsonParser *parser = json_parser_new();
    cursor_animation *animation = NULL;
    if (json_parser_load_from_file(parser, metadata, NULL)) {
        JsonNode *root = json_parser_get_root(parser);
        if (root != NULL && JSON_NODE_HOLDS_ARRAY(root)) {
            animation = load_frames(theme, cursor_dir, json_node_get_array(root));
        }
    }
    g_object_unref(parser);
    g_free(metadata);
    g_free(cursor_dir);
    return animation;
}

static cursor_animation *load_frames(const scalable_theme *theme, const char *cursor_dir,
                                     JsonArray *entries)
{
    guint count = json_array_get_length(entries);
    if (count == 0) {
        return NULL;
    }
    cursor_animation *animation = g_new0(cursor_animation, 1);
    animation->entries = g_new0(frame_entry, count);
    animation->frames = g_new0(cursor_frame, count);
    animation->size = theme->size;
    animation->scale = theme->scale;
    if (!read_entries(animation, cursor_dir, entries) || !render_frame(animation, 0)) {
        animation_free(animation);
        return NULL;
    }
    return animation;
}

static bool read_entries(cursor_animation *animation, const char *cursor_dir,
                         JsonArray *entries)
{
    guint count = json_array_get_length(entries);
    for (guint i = 0; i < count; i++) {
        JsonObject *object = json_array_get_object_element(entries, i);
        if (object == NULL || !read_entry(object, cursor_dir, &animation->entries[i])) {
            return false;
        }
        animation->count++;
    }
    return true;
}

static bool read_entry(JsonObject *object, const char *cursor_dir, frame_entry *entry)
{
    const char *file = json_object_get_string_member_with_default(object, "filename", NULL);
    entry->nominal = json_object_get_double_member_with_default(object, "nominal_size", 0.0);
    if (file == NULL || entry->nominal <= 0.0) {
        return false;
    }
    entry->path = g_build_filename(cursor_dir, file, NULL);
    entry->hotspot_x = json_object_get_double_member_with_default(object, "hotspot_x", 0.0);
    entry->hotspot_y = json_object_get_double_member_with_default(object, "hotspot_y", 0.0);
    entry->delay_ms = (uint32_t) json_object_get_int_member_with_default(object, "delay", 0);
    return true;
}

static bool render_frame(cursor_animation *animation, size_t index)
{
    const frame_entry *entry = &animation->entries[index];
    cursor_frame *frame = &animation->frames[index];
    RsvgHandle *svg = rsvg_handle_new_from_file(entry->path, NULL);
    if (svg == NULL) {
        return false;
    }
    scalable_request request = {canvas_size(svg, entry->nominal), entry->nominal, animation->size,
                                animation->scale};
    scalable_layout layout = scalable_layout_for(&request);
    cairo_surface_t *surface = render_svg(svg, layout.pixels);
    g_object_unref(svg);
    frame->buffer = cairo_buffer_create(surface);
    cairo_surface_destroy(surface);
    frame->scale = layout.buffer_scale;
    frame->hotspot_x = (int32_t) lround(entry->hotspot_x * layout.hotspot_factor);
    frame->hotspot_y = (int32_t) lround(entry->hotspot_y * layout.hotspot_factor);
    frame->delay_ms = entry->delay_ms;
    return frame->buffer != NULL;
}

static cairo_surface_t *render_svg(RsvgHandle *svg, int pixels)
{
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixels, pixels);
    cairo_t *cr = cairo_create(surface);
    RsvgRectangle viewport = {0.0, 0.0, pixels, pixels};
    if (!rsvg_handle_render_document(svg, cr, &viewport, NULL)) {
        wlr_log(WLR_ERROR, "cannot render a scalable cursor");
    }
    cairo_destroy(cr);
    cairo_surface_flush(surface);
    return surface;
}

static double canvas_size(RsvgHandle *svg, double nominal)
{
    gdouble width = 0.0;
    gdouble height = 0.0;
    if (rsvg_handle_get_intrinsic_size_in_pixels(svg, &width, &height) && width > 0.0) {
        return width;
    }
    return nominal;
}

static void animation_free(gpointer data)
{
    cursor_animation *animation = data;
    if (animation == NULL) {
        return;
    }
    for (size_t i = 0; i < animation->count; i++) {
        if (animation->frames[i].buffer != NULL) {
            wlr_buffer_drop(animation->frames[i].buffer);
        }
        g_free(animation->entries[i].path);
    }
    g_free(animation->entries);
    g_free(animation->frames);
    g_free(animation);
}
