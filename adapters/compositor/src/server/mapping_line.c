#include "mapping_line.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

enum {
    PERMISSION_COUNT = 4,
    WRITE_FLAG_INDEX = 1,
    SHARED_FLAG_INDEX = 3,
    PARSED_FIELDS = 3,
};

static const char device_dir[] = "/dev/";
static const char shm_dir[] = "/dev/shm/";

static bool is_plain_file(const char *path);
static bool has_prefix(const char *text, const char *prefix);
static bool is_shared_or_read_only(const char *permissions);

bool mapping_line_parse(const char *maps_line, mapping *mapping)
{
    uintptr_t start = 0;
    uintptr_t end = 0;
    char permissions[PERMISSION_COUNT + 1] = "";
    int path_at = 0;
    int parsed = sscanf(maps_line, "%" SCNxPTR "-%" SCNxPTR " %4s %*s %*s %*s %n", &start, &end,
                        permissions, &path_at);
    if (parsed != PARSED_FIELDS || path_at == 0 || end <= start) {
        return false;
    }
    if (!is_plain_file(maps_line + path_at) || !is_shared_or_read_only(permissions)) {
        return false;
    }
    mapping->start = start;
    mapping->length = end - start;
    return true;
}

static bool is_plain_file(const char *path)
{
    return path[0] == '/' && (!has_prefix(path, device_dir) || has_prefix(path, shm_dir));
}

static bool has_prefix(const char *text, const char *prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static bool is_shared_or_read_only(const char *permissions)
{
    if (strlen(permissions) != PERMISSION_COUNT) {
        return false;
    }
    if (permissions[SHARED_FLAG_INDEX] == 's') {
        return true;
    }
    return permissions[0] == 'r' && permissions[WRITE_FLAG_INDEX] == '-';
}
