#ifndef APPEARANCE_TEST_FIXTURE_H
#define APPEARANCE_TEST_FIXTURE_H

#include <cairo.h>
#include <stdbool.h>
#include <stdint.h>

#include "wm/geometry.h"

char *fixture_dir_create(void);
void fixture_dir_remove(char *dir);
char *fixture_write(const char *dir, const char *name, const char *content);
void fixture_write_png(const char *dir, const char *name, wm_size size);
bool fixture_write_png_pixel(const char *dir, const char *name, uint32_t rgba);
unsigned fixture_alpha_at(cairo_surface_t *surface, int x, int y);

#endif
