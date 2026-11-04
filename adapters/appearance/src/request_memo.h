#ifndef APPEARANCE_REQUEST_MEMO_H
#define APPEARANCE_REQUEST_MEMO_H

#include <stdbool.h>

#include "appearance/appearance.h"

typedef struct request_memo {
    bool is_set;
    char *name;
    char *title_font;
    double max_output_scale;
    ui_style style;
} request_memo;

void appearance_request_memo_store(request_memo *memo, const theme_request *request);
void appearance_request_memo_clear(request_memo *memo);
bool appearance_request_memo_matches(const request_memo *memo, const theme_request *request);

#endif
