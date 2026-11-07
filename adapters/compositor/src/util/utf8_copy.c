#include <stdbool.h>
#include <string.h>

#include "utf8_copy.h"

enum {
    CONTINUATION_MASK = 0xC0,
    CONTINUATION_BITS = 0x80,
    LEAD_TWO = 0xC0,
    LEAD_THREE = 0xE0,
    LEAD_FOUR = 0xF0,
};

static size_t sequence_length(unsigned char lead);
static bool is_continuation(unsigned char byte);
static size_t complete_length(const char *text, size_t length);

void utf8_copy(char *dest, size_t capacity, const char *source)
{
    const char *text = source != NULL ? source : "";
    size_t length = strlen(text);
    if (length >= capacity) {
        length = complete_length(text, capacity - 1);
    }
    memcpy(dest, text, length);
    dest[length] = '\0';
}

static size_t complete_length(const char *text, size_t length)
{
    size_t start = length;
    while (start > 0 && is_continuation((unsigned char) text[start - 1])) {
        start--;
    }
    if (start == 0) {
        return length;
    }
    size_t needed = sequence_length((unsigned char) text[start - 1]);
    return length - (start - 1) < needed ? start - 1 : length;
}

static size_t sequence_length(unsigned char lead)
{
    if (lead >= LEAD_FOUR) {
        return 4;
    }
    if (lead >= LEAD_THREE) {
        return 3;
    }
    return lead >= LEAD_TWO ? 2 : 1;
}

static bool is_continuation(unsigned char byte)
{
    return (byte & CONTINUATION_MASK) == CONTINUATION_BITS;
}
