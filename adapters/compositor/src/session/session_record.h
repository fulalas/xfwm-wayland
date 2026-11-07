#ifndef COMPOSITOR_SESSION_RECORD_H
#define COMPOSITOR_SESSION_RECORD_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/geometry.h"

enum {
    SESSION_ID_LENGTH = 32,
};

typedef struct session_record {
    char *name;
    wm_rect geometry;
    uint32_t states;
    bool has_workspace;
    int workspace;
    int64_t seen;
} session_record;

extern const char session_file_header[];

bool session_id_is_valid(const char *id);
bool session_is_expired(int64_t last_used, int64_t now);
char *session_record_format(const session_record *record);
bool session_record_parse(const char *line, session_record *record);
void session_record_finish(session_record *record);

#endif
