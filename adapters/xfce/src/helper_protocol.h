#ifndef XFCE_HELPER_PROTOCOL_H
#define XFCE_HELPER_PROTOCOL_H

#include <stdbool.h>

#include "compositor/services.h"
#include "xfce/environment.h"

enum {
    HELPER_TEXT_MAX = 4096,
};

typedef enum helper_message_kind {
    HELPER_MESSAGE_STYLE_BEGIN,
    HELPER_MESSAGE_SYMBOL,
    HELPER_MESSAGE_COLOR,
    HELPER_MESSAGE_FONT,
    HELPER_MESSAGE_STYLE_END,
    HELPER_MESSAGE_ICON,
} helper_message_kind;

typedef struct helper_message {
    helper_message_kind kind;
    char name[XFCE_SYMBOL_NAME_MAX];
    char text[HELPER_TEXT_MAX];
    unsigned id;
} helper_message;

bool helper_message_parse(const char *line, helper_message *message);
bool helper_color_parse(const char *text, xfce_color *color);
char *helper_icon_request_format(unsigned id, const icon_request *request);
bool helper_terminate_answer(const char *output);

#endif
