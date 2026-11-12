#ifndef COMPOSITOR_XCURSOR_FILE_H
#define COMPOSITOR_XCURSOR_FILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct xcursor_frame {
    uint32_t width;
    uint32_t height;
    uint32_t hotspot_x;
    uint32_t hotspot_y;
    uint32_t delay;
    uint32_t *pixels;
} xcursor_frame;

typedef struct xcursor_frames {
    xcursor_frame *frames;
    size_t count;
    bool larger;
} xcursor_frames;

typedef struct xcursor_request {
    int size;
    int width;
    int larger;
} xcursor_request;

bool xcursor_file_load(FILE *file, const xcursor_request *request, xcursor_frames *frames);
char **xcursor_search_path(void);
char **xcursor_theme_dirs(const char *theme);
bool xcursor_theme_load(char *const *dirs, const char *name, const xcursor_request *request,
                        xcursor_frames *frames);
void xcursor_frames_clear(xcursor_frames *frames);

#endif
