#include <math.h>

#include "compositor/services.h"
#include "input_transform.h"

enum {
    QUARTER_TURN_DEGREES = 90,
    TURNS_PER_CIRCLE = 4,
    OUTPUT_ROTATION_MASK = 3,
    OUTPUT_FLIPPED_BIT = 4,
};

static const input_matrix quarter_turns[TURNS_PER_CIRCLE] = {
    { { 1, 0, 0, 0, 1, 0 } },
    { { 0, -1, 1, 1, 0, 0 } },
    { { -1, 0, 1, 0, -1, 1 } },
    { { 0, 1, 0, -1, 0, 1 } },
};

static const input_matrix mirror_x = { { -1, 0, 1, 0, 1, 0 } };
static const input_matrix mirror_y = { { 1, 0, 0, 0, -1, 1 } };

static input_matrix reflection_matrix(unsigned reflection);
static int quarter_turns_of(int degrees);

input_matrix input_matrix_identity(void)
{
    return quarter_turns[0];
}

input_matrix input_matrix_for_device(int rotation_degrees, unsigned reflection)
{
    input_matrix reflect = reflection_matrix(reflection);
    return input_matrix_then(&reflect, &quarter_turns[quarter_turns_of(rotation_degrees)]);
}

input_matrix input_matrix_for_output(int output_transform)
{
    const input_matrix *rotate = &quarter_turns[output_transform & OUTPUT_ROTATION_MASK];
    if ((output_transform & OUTPUT_FLIPPED_BIT) == 0) {
        return *rotate;
    }
    return input_matrix_then(rotate, &mirror_x);
}

input_matrix input_matrix_then(const input_matrix *first, const input_matrix *second)
{
    const double *a = second->m;
    const double *b = first->m;
    return (input_matrix){{
        a[0] * b[0] + a[1] * b[3],
        a[0] * b[1] + a[1] * b[4],
        a[0] * b[2] + a[1] * b[5] + a[2],
        a[3] * b[0] + a[4] * b[3],
        a[3] * b[1] + a[4] * b[4],
        a[3] * b[2] + a[4] * b[5] + a[5],
    }};
}

input_point input_matrix_apply(const input_matrix *matrix, input_point point)
{
    input_point turned = input_matrix_turn(matrix, point);
    return (input_point){turned.x + matrix->m[2], turned.y + matrix->m[5]};
}

input_point input_matrix_turn(const input_matrix *matrix, input_point direction)
{
    const double *m = matrix->m;
    return (input_point){
        m[0] * direction.x + m[1] * direction.y,
        m[3] * direction.x + m[4] * direction.y,
    };
}

static input_matrix reflection_matrix(unsigned reflection)
{
    input_matrix result = input_matrix_identity();
    if ((reflection & INPUT_REFLECT_X) != 0) {
        result = input_matrix_then(&result, &mirror_x);
    }
    if ((reflection & INPUT_REFLECT_Y) != 0) {
        result = input_matrix_then(&result, &mirror_y);
    }
    return result;
}

static int quarter_turns_of(int degrees)
{
    double turns = round((double) degrees / QUARTER_TURN_DEGREES);
    int wrapped = (int) fmod(turns, TURNS_PER_CIRCLE);
    return wrapped < 0 ? wrapped + TURNS_PER_CIRCLE : wrapped;
}
