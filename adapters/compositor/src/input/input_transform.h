#ifndef COMPOSITOR_INPUT_TRANSFORM_H
#define COMPOSITOR_INPUT_TRANSFORM_H

enum {
    INPUT_MATRIX_VALUES = 6,
};

typedef struct input_point {
    double x;
    double y;
} input_point;

typedef struct input_matrix {
    double m[INPUT_MATRIX_VALUES];
} input_matrix;

input_matrix input_matrix_identity(void);
input_matrix input_matrix_for_device(int rotation_degrees, unsigned reflection);
input_matrix input_matrix_for_output(int output_transform);
input_matrix input_matrix_then(const input_matrix *first, const input_matrix *second);
input_point input_matrix_apply(const input_matrix *matrix, input_point point);
input_point input_matrix_turn(const input_matrix *matrix, input_point direction);

#endif
