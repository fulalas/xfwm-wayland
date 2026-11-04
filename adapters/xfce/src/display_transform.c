#include "display_transform.h"

#include <string.h>

enum {
    QUARTER_TURN_DEGREES = 90,
    QUARTER_TURNS = 4,
    REFLECTION_COUNT = 4,
};

static const char *const reflections[REFLECTION_COUNT] = { "0", "X", "Y", "XY" };

static const display_transform transforms[REFLECTION_COUNT][QUARTER_TURNS] = {
    { DISPLAY_TRANSFORM_NORMAL, DISPLAY_TRANSFORM_90, DISPLAY_TRANSFORM_180,
      DISPLAY_TRANSFORM_270 },
    { DISPLAY_TRANSFORM_FLIPPED, DISPLAY_TRANSFORM_FLIPPED_90, DISPLAY_TRANSFORM_FLIPPED_180,
      DISPLAY_TRANSFORM_FLIPPED_270 },
    { DISPLAY_TRANSFORM_FLIPPED_180, DISPLAY_TRANSFORM_FLIPPED_270, DISPLAY_TRANSFORM_FLIPPED,
      DISPLAY_TRANSFORM_FLIPPED_90 },
    { DISPLAY_TRANSFORM_180, DISPLAY_TRANSFORM_270, DISPLAY_TRANSFORM_NORMAL,
      DISPLAY_TRANSFORM_90 },
};

static int reflection_index(const char *reflection);
static int quarter_turns(int degrees);

display_transform display_transform_from(const display_rotation *rotation)
{
    return transforms[reflection_index(rotation->reflection)][quarter_turns(rotation->degrees)];
}

display_rotation display_rotation_from(display_transform transform)
{
    int turns = (int) transform % QUARTER_TURNS;
    bool is_flipped = transform >= DISPLAY_TRANSFORM_FLIPPED;

    return (display_rotation) {
        .degrees = turns * QUARTER_TURN_DEGREES,
        .reflection = reflections[is_flipped ? 1 : 0],
    };
}

static int reflection_index(const char *reflection)
{
    for (int i = 0; reflection && i < REFLECTION_COUNT; i++) {
        if (strcmp(reflection, reflections[i]) == 0) {
            return i;
        }
    }
    return 0;
}

static int quarter_turns(int degrees)
{
    int turns = degrees / QUARTER_TURN_DEGREES;

    return degrees % QUARTER_TURN_DEGREES == 0 && turns >= 0 && turns < QUARTER_TURNS ? turns : 0;
}
