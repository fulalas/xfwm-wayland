#ifndef COMPOSITOR_INPUT_MAPPING_H
#define COMPOSITOR_INPUT_MAPPING_H

#include <stdbool.h>
#include <stdint.h>

typedef struct axis_steps {
    int32_t remainder;
    int steps;
} axis_steps;

typedef struct key_chord {
    uint32_t translated_keysym;
    uint32_t base_keysym;
    uint32_t modifiers;
} key_chord;

uint32_t input_mapping_modifiers(uint32_t wlr_modifiers);
uint32_t input_mapping_keysym(uint32_t level0_keysym);
unsigned input_mapping_vt(const key_chord *chord);
bool input_mapping_shared_with_x11(const key_chord *chord);
uint32_t input_mapping_button(uint32_t linux_button);
axis_steps input_mapping_axis(int32_t remainder, int32_t value120);
int32_t input_mapping_axis_value120(int32_t value120, double delta);
uint32_t input_mapping_edges_from_wlr(uint32_t wlr_edges);
uint32_t input_mapping_edges_to_wlr(uint32_t wm_edges);

#endif
