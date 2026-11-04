#include "xsettings_wire.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum {
    HEADER_SIZE = 12,
    WORD = 4,
    BYTE_ORDER_LSB_FIRST = 0,
    BYTE_ORDER_MSB_FIRST = 1,
    WIRE_TYPE_INTEGER = 0,
    WIRE_TYPE_STRING = 1,
    SETTING_PREFIX_SIZE = 4,
};

typedef struct writer {
    uint8_t *data;
    size_t position;
} writer;

static size_t padded(size_t length);
static size_t encoded_size(const x11_setting *setting);
static void put_header(writer *w, uint32_t serial, uint32_t count);
static void put_setting(writer *w, const x11_setting *setting, uint32_t serial);
static void put_u8(writer *w, uint8_t value);
static void put_u16(writer *w, uint16_t value);
static void put_u32(writer *w, uint32_t value);
static void put_text(writer *w, const char *text, size_t length);

xsettings_bytes xsettings_wire_encode(const xsettings_table *table, uint32_t serial)
{
    size_t count = xsettings_table_count(table);
    size_t size = HEADER_SIZE;
    for (size_t i = 0; i < count; i++) {
        size += encoded_size(xsettings_table_at(table, i));
    }
    writer w = {calloc(1, size), 0};
    if (w.data == NULL) {
        return (xsettings_bytes){NULL, 0};
    }
    put_header(&w, serial, (uint32_t) count);
    for (size_t i = 0; i < count; i++) {
        put_setting(&w, xsettings_table_at(table, i), serial);
    }
    return (xsettings_bytes){w.data, size};
}

void xsettings_bytes_free(xsettings_bytes *bytes)
{
    free(bytes->data);
    bytes->data = NULL;
    bytes->length = 0;
}

static size_t padded(size_t length)
{
    return (length + WORD - 1) / WORD * WORD;
}

static size_t encoded_size(const x11_setting *setting)
{
    size_t size = SETTING_PREFIX_SIZE + padded(strlen(setting->name)) + WORD;
    if (setting->type == X11_SETTING_STRING) {
        return size + WORD + padded(strlen(setting->string));
    }
    return size + WORD;
}

static void put_header(writer *w, uint32_t serial, uint32_t count)
{
    const uint16_t probe = 1;
    bool lsb_first = *(const uint8_t *) &probe == 1;
    put_u8(w, lsb_first ? BYTE_ORDER_LSB_FIRST : BYTE_ORDER_MSB_FIRST);
    w->position += 3;
    put_u32(w, serial);
    put_u32(w, count);
}

static void put_setting(writer *w, const x11_setting *setting, uint32_t serial)
{
    bool is_string = setting->type == X11_SETTING_STRING;
    size_t name_length = strlen(setting->name);
    put_u8(w, is_string ? WIRE_TYPE_STRING : WIRE_TYPE_INTEGER);
    w->position += 1;
    put_u16(w, (uint16_t) name_length);
    put_text(w, setting->name, name_length);
    put_u32(w, serial);
    if (is_string) {
        size_t value_length = strlen(setting->string);
        put_u32(w, (uint32_t) value_length);
        put_text(w, setting->string, value_length);
        return;
    }
    put_u32(w, (uint32_t) setting->integer);
}

static void put_u8(writer *w, uint8_t value)
{
    w->data[w->position++] = value;
}

static void put_u16(writer *w, uint16_t value)
{
    memcpy(w->data + w->position, &value, sizeof(value));
    w->position += sizeof(value);
}

static void put_u32(writer *w, uint32_t value)
{
    memcpy(w->data + w->position, &value, sizeof(value));
    w->position += sizeof(value);
}

static void put_text(writer *w, const char *text, size_t length)
{
    memcpy(w->data + w->position, text, length);
    w->position += padded(length);
}
