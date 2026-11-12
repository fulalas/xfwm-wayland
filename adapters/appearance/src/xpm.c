#include "xpm.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "color.h"
#include "image_limits.h"
#include "xpm_reader.h"

enum {
    HEADER_FIELDS = 4,
    CHARS_PER_PIXEL_MAX = 32,
    CHANNEL_SHIFT = 8,
    BITS_PER_SAMPLE = 8,
    ALPHA_OPAQUE = 0xFF,
    PIXEL_BYTES = 4,
};

typedef struct xpm_header {
    int width;
    int height;
    int color_count;
    int chars_per_pixel;
} xpm_header;

typedef struct xpm_pixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} xpm_pixel;

typedef struct xpm_palette {
    GHashTable *colors;
    xpm_pixel fallback;
} xpm_palette;

typedef struct xpm_decoder {
    xpm_reader *reader;
    const xpm_symbols *symbols;
    xpm_header header;
    xpm_palette palette;
} xpm_decoder;

static GdkPixbuf *read_image(xpm_decoder *decoder);
static bool read_header(xpm_reader *reader, xpm_header *header);
static bool header_is_valid(const xpm_header *header);
static bool read_palette(xpm_decoder *decoder);
static xpm_pixel entry_pixel(const char *entry, const xpm_symbols *symbols);
static void read_rows(const xpm_decoder *decoder, GdkPixbuf *pixbuf);
static void decode_row(const xpm_decoder *decoder, const char *row, guchar *pixels);

GdkPixbuf *appearance_xpm_load(const char *path, const xpm_symbols *symbols)
{
    xpm_decoder decoder = { .reader = appearance_xpm_reader_create(path), .symbols = symbols };

    if (decoder.reader == NULL) {
        return NULL;
    }
    GdkPixbuf *pixbuf = read_image(&decoder);
    appearance_xpm_reader_destroy(decoder.reader);
    return pixbuf;
}

static GdkPixbuf *read_image(xpm_decoder *decoder)
{
    const xpm_header *header = &decoder->header;

    if (!read_header(decoder->reader, &decoder->header) || !read_palette(decoder)) {
        return NULL;
    }
    GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, BITS_PER_SAMPLE, header->width,
                                       header->height);
    if (pixbuf == NULL) {
        g_warning("Cannot allocate %dx%d XPM pixbuf", header->width, header->height);
    } else {
        read_rows(decoder, pixbuf);
    }
    g_hash_table_destroy(decoder->palette.colors);
    return pixbuf;
}

static bool read_header(xpm_reader *reader, xpm_header *header)
{
    const char *text = appearance_xpm_reader_header(reader);

    if (text == NULL) {
        g_warning("Cannot read XPM header");
        return false;
    }
    int fields = sscanf(text, "%d %d %d %d", &header->width, &header->height,
                        &header->color_count, &header->chars_per_pixel);
    if (fields != HEADER_FIELDS) {
        g_warning("XPM header has %d attributes, expected %d", fields, HEADER_FIELDS);
        return false;
    }
    if (!header_is_valid(header)) {
        g_warning("XPM header contains invalid attributes");
        return false;
    }
    return true;
}

static bool header_is_valid(const xpm_header *header)
{
    return header->width > 0 && header->height > 0 && header->width <= APPEARANCE_IMAGE_SIDE_MAX
           && header->height <= APPEARANCE_IMAGE_SIDE_MAX && header->chars_per_pixel > 0
           && header->chars_per_pixel < CHARS_PER_PIXEL_MAX && header->color_count > 0
           && header->color_count < INT_MAX / (header->chars_per_pixel + 1)
           && header->color_count < INT_MAX / (int) sizeof(xpm_pixel);
}

static bool read_palette(xpm_decoder *decoder)
{
    xpm_palette *palette = &decoder->palette;

    palette->colors = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    for (int i = 0; i < decoder->header.color_count; i++) {
        const char *entry = appearance_xpm_reader_colormap(decoder->reader);
        if (entry == NULL) {
            g_warning("Cannot read XPM colormap entry %d", i);
            g_hash_table_destroy(palette->colors);
            return false;
        }
        char *code = g_strndup(entry, (gsize) decoder->header.chars_per_pixel);
        xpm_pixel pixel = entry_pixel(entry + strlen(code), decoder->symbols);
        if (i == 0) {
            palette->fallback = pixel;
        }
        g_hash_table_insert(palette->colors, code, g_memdup2(&pixel, sizeof pixel));
    }
    return true;
}

static xpm_pixel entry_pixel(const char *entry, const xpm_symbols *symbols)
{
    char *name = appearance_xpm_entry_color(entry, symbols);
    xpm_pixel pixel = { 0 };
    xpm_rgb rgb;

    if (name != NULL && g_ascii_strcasecmp(name, "None") != 0
        && appearance_xpm_color_parse(name, &rgb)) {
        pixel = (xpm_pixel) {
            (uint8_t) (rgb.red >> CHANNEL_SHIFT), (uint8_t) (rgb.green >> CHANNEL_SHIFT),
            (uint8_t) (rgb.blue >> CHANNEL_SHIFT), ALPHA_OPAQUE,
        };
    }
    g_free(name);
    return pixel;
}

static void read_rows(const xpm_decoder *decoder, GdkPixbuf *pixbuf)
{
    const xpm_header *header = &decoder->header;
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
    int stride = gdk_pixbuf_get_rowstride(pixbuf);
    size_t row_chars = (size_t) header->width * (size_t) header->chars_per_pixel;

    for (int y = 0; y < header->height; y++) {
        const char *row = appearance_xpm_reader_row(decoder->reader);
        guchar *line = pixels + (size_t) y * (size_t) stride;
        if (row == NULL || strlen(row) < row_chars) {
            memset(line, 0, (size_t) header->width * PIXEL_BYTES);
            continue;
        }
        decode_row(decoder, row, line);
    }
}

static void decode_row(const xpm_decoder *decoder, const char *row, guchar *pixels)
{
    const xpm_palette *palette = &decoder->palette;
    char code[CHARS_PER_PIXEL_MAX];
    size_t step = (size_t) decoder->header.chars_per_pixel;

    for (int x = 0; x < decoder->header.width; x++) {
        memcpy(code, row + (size_t) x * step, step);
        code[step] = '\0';
        const xpm_pixel *pixel = g_hash_table_lookup(palette->colors, code);
        if (pixel == NULL) {
            pixel = &palette->fallback;
        }
        memcpy(pixels + (size_t) x * PIXEL_BYTES, pixel, PIXEL_BYTES);
    }
}
