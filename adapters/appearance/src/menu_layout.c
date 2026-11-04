#include "menu_layout.h"

#include "menu_labels.h"

enum {
    MENU_PADDING_Y = 4,
    ITEM_PADDING_Y = 4,
    ITEM_MIN_HEIGHT = 16,
    ITEM_MIN_WIDTH = 40,
    TOGGLE_SPACING = 7,
    ARROW_SPACING = 10,
    SEPARATOR_MARGIN = 3,
    SEPARATOR_LINE = 1,
};

typedef struct menu_rows {
    const wm_menu_level *level;
    int item_height;
} menu_rows;

static int item_height(const text_font *font);
static int menu_width(const text_font *font, const wm_menu_level *level);
static int menu_height(const menu_rows *rows);
static int text_width(const text_font *font, const wm_menu_level *level, size_t index);
static int widest_label(const text_font *font, const wm_menu_level *level);
static bool any_mark(const wm_menu_level *level);
static bool any_submenu(const wm_menu_level *level);
static int row_height(const menu_rows *rows, size_t index);

menu_layout appearance_menu_layout(const text_font *font, const wm_menu_level *level)
{
    menu_rows rows = { level, item_height(font) };
    menu_layout layout = {
        .size = { menu_width(font, level), menu_height(&rows) },
        .count = appearance_menu_item_count(level),
    };
    int y = MENU_BORDER + MENU_PADDING_Y;

    for (size_t i = 0; i < layout.count; i++) {
        int height = row_height(&rows, i);
        layout.items[i] = (wm_rect) { MENU_BORDER, y, layout.size.width - 2 * MENU_BORDER, height };
        y += height;
    }
    return layout;
}

menu_columns appearance_menu_columns(const wm_menu_level *level)
{
    return (menu_columns) {
        .toggle_width = any_mark(level) ? MENU_TOGGLE_SIZE + TOGGLE_SPACING : 0,
        .arrow_width = any_submenu(level) ? MENU_ARROW_SIZE + ARROW_SPACING : 0,
    };
}

wm_rect appearance_menu_label_rect(const menu_columns *columns, const wm_rect *item)
{
    int left = MENU_ITEM_PADDING_X + columns->toggle_width;
    int right = MENU_ITEM_PADDING_X + columns->arrow_width;

    return (wm_rect) { item->x + left, item->y + ITEM_PADDING_Y, item->width - left - right,
                       item->height - 2 * ITEM_PADDING_Y };
}

size_t appearance_menu_item_count(const wm_menu_level *level)
{
    return MIN(level->count, WM_MENU_ITEM_MAX);
}

bool appearance_menu_is_separator(const wm_menu_level *level, size_t index)
{
    return level->items[index].label == WM_MENU_SEPARATOR;
}

static int item_height(const text_font *font)
{
    int text_height = appearance_text_line_height(font);

    return wm_max(text_height, ITEM_MIN_HEIGHT) + 2 * ITEM_PADDING_Y;
}

static int menu_width(const text_font *font, const wm_menu_level *level)
{
    menu_columns columns = appearance_menu_columns(level);
    int content = columns.toggle_width + widest_label(font, level) + columns.arrow_width;
    int item_width = content + 2 * MENU_ITEM_PADDING_X;

    return wm_max(item_width, ITEM_MIN_WIDTH) + 2 * MENU_BORDER;
}

static int menu_height(const menu_rows *rows)
{
    int height = 2 * (MENU_BORDER + MENU_PADDING_Y);

    for (size_t i = 0; i < appearance_menu_item_count(rows->level); i++) {
        height += row_height(rows, i);
    }
    return height;
}

static int text_width(const text_font *font, const wm_menu_level *level, size_t index)
{
    char *text = appearance_menu_item_text(level, index);
    PangoLayout *layout = appearance_text_layout(font, text);
    int width = appearance_text_layout_size(layout).width;

    g_object_unref(layout);
    g_free(text);
    return width;
}

static int widest_label(const text_font *font, const wm_menu_level *level)
{
    int widest = 0;

    for (size_t i = 0; i < appearance_menu_item_count(level); i++) {
        int width = appearance_menu_is_separator(level, i) ? 0 : text_width(font, level, i);
        widest = wm_max(width, widest);
    }
    return widest;
}

static bool any_mark(const wm_menu_level *level)
{
    for (size_t i = 0; i < appearance_menu_item_count(level); i++) {
        if (level->items[i].mark != WM_MENU_MARK_NONE) {
            return true;
        }
    }
    return false;
}

static bool any_submenu(const wm_menu_level *level)
{
    for (size_t i = 0; i < appearance_menu_item_count(level); i++) {
        if (level->items[i].has_submenu) {
            return true;
        }
    }
    return false;
}

static int row_height(const menu_rows *rows, size_t index)
{
    if (appearance_menu_is_separator(rows->level, index)) {
        return 2 * SEPARATOR_MARGIN + SEPARATOR_LINE;
    }
    return rows->item_height;
}
