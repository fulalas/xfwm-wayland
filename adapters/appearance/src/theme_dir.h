#ifndef APPEARANCE_THEME_DIR_H
#define APPEARANCE_THEME_DIR_H

typedef struct theme_location {
    char *dir;
    int pixmap_scale;
} theme_location;

theme_location appearance_theme_locate(const char *name, double max_output_scale);
void appearance_theme_location_clear(theme_location *location);

#endif
