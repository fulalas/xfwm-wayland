#include "scale_guess.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

enum {
    SIZE_MIN_MM = 4,
    TV_HEIGHT_MM = 500,
    DPI_MIN = 50,
    DPI_MAX = 500,
    SMALL_PANEL_INCHES = 20,
    SMALL_PANEL_DPI = 135,
    LARGE_PANEL_DPI = 110,
};

static const double MM_PER_INCH = 25.4;
static const double SCALE_STEP = 0.25;
static const double SCALE_GUESS_MIN = 1.25;
static const double SCALE_MAX = 3.0;
static const double SCALE_NONE = 1.0;

static const wm_size aspect_only_sizes[] = {
    {16, 9}, {16, 10}, {160, 90}, {160, 100}, {1600, 900}, {1600, 1000},
};

static bool is_real_size(wm_size millimeters);
static bool is_aspect_only(wm_size millimeters);
static double diagonal(wm_size size);
static double round_scale(double raw);

double scale_guess(const scale_guess_panel *panel)
{
    if (!is_real_size(panel->millimeters) || panel->millimeters.height > TV_HEIGHT_MM) {
        return SCALE_NONE;
    }
    double inches = diagonal(panel->millimeters) / MM_PER_INCH;
    double dpi = diagonal(panel->pixels) / inches;
    if (dpi < DPI_MIN || dpi > DPI_MAX) {
        return SCALE_NONE;
    }
    double target = inches < SMALL_PANEL_INCHES ? SMALL_PANEL_DPI : LARGE_PANEL_DPI;
    return round_scale(dpi / target);
}

static bool is_real_size(wm_size millimeters)
{
    if (millimeters.width < SIZE_MIN_MM || millimeters.height < SIZE_MIN_MM) {
        return false;
    }
    return !is_aspect_only(millimeters);
}

static bool is_aspect_only(wm_size millimeters)
{
    size_t count = sizeof(aspect_only_sizes) / sizeof(aspect_only_sizes[0]);
    for (size_t i = 0; i < count; i++) {
        if (millimeters.width == aspect_only_sizes[i].width &&
            millimeters.height == aspect_only_sizes[i].height) {
            return true;
        }
    }
    return false;
}

static double diagonal(wm_size size)
{
    return hypot(size.width, size.height);
}

static double round_scale(double raw)
{
    double scale = round(raw / SCALE_STEP) * SCALE_STEP;
    if (scale < SCALE_GUESS_MIN) {
        return SCALE_NONE;
    }
    return fmin(scale, SCALE_MAX);
}
