#include <string.h>

#include "decor_private.h"

enum {
    NO_BUTTON = -1,
    TITLE_SEPARATOR = WM_BUTTON_COUNT,
};

static const char button_letters[] = "OTSHMC";
static const char separator_letter = '|';

typedef struct menu_search {
    bool found;
    int x;
    size_t separator;
} menu_search;

typedef struct button_row {
    const decor_frame *frame;
    decor_buttons *buttons;
    int x;
    int limit;
} button_row;

static size_t place_left_buttons(button_row *row, size_t length);
static void place_right_buttons(button_row *row, size_t separator, size_t length);
static void place_left_button(button_row *row, wm_button button);
static void place_right_button(button_row *row, wm_button button);
static void show_button(button_row *row, wm_button button);
static int button_at(const decor_frame *frame, size_t index);
static int button_from_letter(const wm_decor_window *window, char letter);
static bool button_available(const wm_decor_window *window, wm_button button);
static size_t layout_length(const decor_frame *frame);
static menu_search search_left_menu(const decor_frame *frame);
static int right_menu_x(const decor_frame *frame, size_t separator);
static wm_size decor_button_base_size(const decor_frame *frame, wm_button button);

decor_buttons decor_buttons_compute(const decor_frame *frame)
{
    decor_buttons buttons = {0};
    button_row row = {frame, &buttons, 0, 0};
    size_t length = layout_length(frame);
    size_t separator = place_left_buttons(&row, length);
    place_right_buttons(&row, separator, length);
    return buttons;
}

int decor_menu_button_x(const decor_frame *frame)
{
    menu_search left = search_left_menu(frame);
    if (left.found) {
        return left.x;
    }
    return right_menu_x(frame, left.separator);
}

static size_t place_left_buttons(button_row *row, size_t length)
{
    const decor_frame *frame = row->frame;
    size_t i;

    row->x = frame->extents.left + decor_button_offset(frame);
    row->x = row->x < 0 ? 0 : row->x;
    row->limit = frame->size.width - frame->extents.right - decor_button_offset(frame);
    for (i = 0; i < length; i++) {
        int button = button_at(frame, i);
        if (button == TITLE_SEPARATOR) {
            break;
        }
        if (button != NO_BUTTON) {
            place_left_button(row, (wm_button) button);
        }
    }
    row->buttons->title_left = row->x + frame->config->button_spacing;
    return i;
}

static void place_right_buttons(button_row *row, size_t separator, size_t length)
{
    const decor_frame *frame = row->frame;
    int spacing = frame->config->button_spacing;

    row->x = frame->size.width - frame->extents.right + spacing - decor_button_offset(frame);
    row->limit = row->buttons->title_left;
    for (size_t j = length; j > separator; j--) {
        int button = button_at(frame, j - 1);
        if (button == TITLE_SEPARATOR) {
            break;
        }
        if (button != NO_BUTTON) {
            place_right_button(row, (wm_button) button);
        }
    }
    row->buttons->title_left -= 2 * spacing;
    row->buttons->title_right = row->x;
}

static void place_left_button(button_row *row, wm_button button)
{
    int advance = decor_button_base_size(row->frame, button).width
                  + row->frame->config->button_spacing;
    if (row->x + advance < row->limit) {
        show_button(row, button);
        row->x += advance;
        return;
    }
    row->buttons->visible[button] = false;
}

static void place_right_button(button_row *row, wm_button button)
{
    int advance = decor_button_base_size(row->frame, button).width
                  + row->frame->config->button_spacing;
    if (row->x - advance > row->limit) {
        row->x -= advance;
        show_button(row, button);
        return;
    }
    row->buttons->visible[button] = false;
}

static void show_button(button_row *row, wm_button button)
{
    const decor_frame *frame = row->frame;
    wm_size size = decor_button_base_size(frame, button);
    int y = (frame->title_height - size.height + 1) / 2 - frame->border_top;

    row->buttons->visible[button] = true;
    row->buttons->rect[button] = (wm_rect) {row->x, y, size.width, size.height};
}

static int button_at(const decor_frame *frame, size_t index)
{
    return button_from_letter(frame->window, frame->config->button_layout[index]);
}

static int button_from_letter(const wm_decor_window *window, char letter)
{
    if (letter == separator_letter) {
        return TITLE_SEPARATOR;
    }
    const char *found = letter == '\0' ? NULL : strchr(button_letters, letter);
    if (found == NULL) {
        return NO_BUTTON;
    }
    wm_button button = (wm_button) (found - button_letters);
    return button_available(window, button) ? (int) button : NO_BUTTON;
}

static bool button_available(const wm_decor_window *window, wm_button button)
{
    switch (button) {
    case WM_BUTTON_MENU:
        return window->has_menu;
    case WM_BUTTON_STICK:
        return window->has_stick && window->has_menu;
    case WM_BUTTON_HIDE:
        return window->can_hide;
    case WM_BUTTON_MAXIMIZE:
        return window->can_maximize;
    case WM_BUTTON_CLOSE:
        return window->has_close;
    default:
        return true;
    }
}

static size_t layout_length(const decor_frame *frame)
{
    const char *layout = frame->config->button_layout;
    size_t length = 0;
    while (length < WM_BUTTON_LAYOUT_MAX && layout[length] != '\0') {
        length++;
    }
    return length;
}

static menu_search search_left_menu(const decor_frame *frame)
{
    size_t length = layout_length(frame);
    menu_search search = {false, decor_button_offset(frame), 0};

    for (; search.separator < length; search.separator++) {
        int button = button_at(frame, search.separator);
        if (button == TITLE_SEPARATOR) {
            break;
        }
        if (button == WM_BUTTON_MENU) {
            search.found = true;
            break;
        }
        if (button != NO_BUTTON) {
            search.x += decor_button_base_size(frame, (wm_button) button).width
                        + frame->config->button_spacing;
        }
    }
    return search;
}

static int right_menu_x(const decor_frame *frame, size_t separator)
{
    int x = -decor_button_offset(frame);

    for (size_t j = layout_length(frame); j > separator; j--) {
        int button = button_at(frame, j - 1);
        if (button == TITLE_SEPARATOR) {
            break;
        }
        if (button == WM_BUTTON_MENU) {
            return x - decor_button_base_size(frame, WM_BUTTON_MENU).width;
        }
        if (button != NO_BUTTON) {
            x -= decor_button_base_size(frame, (wm_button) button).width
                 + frame->config->button_spacing;
        }
    }
    return 0;
}

static wm_size decor_button_base_size(const decor_frame *frame, wm_button button)
{
    return frame->theme->button[button][frame->state];
}
