#ifndef DECOR_FIXTURE_H
#define DECOR_FIXTURE_H

#include "wm/decor.h"

enum {
    FIXTURE_SIDE = 4,
    FIXTURE_TITLE_HEIGHT = 20,
    FIXTURE_TOP_CORNER_WIDTH = 6,
    FIXTURE_BOTTOM_CORNER = 6,
    FIXTURE_BUTTON = 16,
    FIXTURE_CLIENT_WIDTH = 200,
    FIXTURE_CLIENT_HEIGHT = 100,
    FIXTURE_TEXT_WIDTH = 50,
    FIXTURE_TEXT_HEIGHT = 10,
};

typedef struct decor_fixture {
    wm_theme_metrics theme;
    wm_frame_config config;
    wm_decor_window window;
} decor_fixture;

decor_fixture fixture_create(void);
void fixture_set_layout(decor_fixture *fixture, const char *button_layout);
void fixture_maximize(decor_fixture *fixture);
void fixture_set_all_states(wm_size sizes[WM_FRAME_STATE_COUNT], wm_size size);
wm_decor_input fixture_input(const decor_fixture *fixture);
wm_decor_layout fixture_layout(const decor_fixture *fixture);
const wm_paint_op *fixture_find_op(const wm_decor_layout *layout, wm_pixmap_kind kind, int index);
const wm_part_rect *fixture_find_part(const wm_decor_layout *layout, wm_part part);
size_t fixture_count_ops(const wm_decor_layout *layout, wm_pixmap_kind kind);
wm_button_image fixture_button_image(const decor_fixture *fixture, wm_button button);

#endif
