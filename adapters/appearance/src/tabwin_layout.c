#include "tabwin_layout.h"

#include <glib.h>

enum {
    TABWIN_PADDING = 4,
    TABWIN_INSET = TABWIN_RADIUS + 2 * (TABWIN_BORDER + TABWIN_PADDING),
    CELL_SPACING = 4,
    LABEL_SPACING = 3,
    ICON_BORDER = 5,
    ROW_PADDING = 4,
    ROW_TEXT_SPACING = 6,
    PREVIEW_ICON_DIVISOR = 4,
    PREVIEW_ICON_MAX = 48,
};

typedef struct tabwin_grid {
    const wm_cycle_view *view;
    int columns;
    int rows;
    wm_size cell;
    int label_height;
} tabwin_grid;

static tabwin_grid grid_for(const wm_cycle_view *view, int label_height);
static wm_size panel_size(const tabwin_grid *grid);
static wm_rect cell_rect(const tabwin_grid *grid, size_t index);
static wm_rect preview_rect(const wm_cycle_view *view, const wm_rect *cell);
static int count_of(const wm_cycle_view *view);
static int ceil_div(int value, int divisor);
static wm_size grid_cell(const wm_cycle_view *view, int label_height);
static wm_size list_cell(const wm_cycle_view *view, int columns, int label_height);
static wm_size grid_extent(const tabwin_grid *grid);
static wm_rect centered_square(const wm_rect *cell, int size, int top);
static int preview_icon_size(int item_size);
static wm_rect icon_rect(const wm_cycle_view *view, const wm_rect *cell);

cycle_layout appearance_tabwin_layout(const wm_cycle_view *view, int label_height)
{
    tabwin_grid grid = grid_for(view, label_height);
    cycle_layout layout = {
        .size = panel_size(&grid),
        .count = appearance_tabwin_item_count(view),
    };

    for (size_t i = 0; i < layout.count; i++) {
        layout.items[i] = cell_rect(&grid, i);
        layout.previews[i] = preview_rect(view, &layout.items[i]);
        layout.icons[i] = icon_rect(view, &layout.items[i]);
    }
    return layout;
}

static wm_rect icon_rect(const wm_cycle_view *view, const wm_rect *cell)
{
    int item = view->item_size;

    if (view->mode == WM_TABWIN_LIST) {
        return (wm_rect) { cell->x + ROW_TEXT_SPACING, cell->y + (cell->height - item) / 2, item,
                           item };
    }
    if (!view->preview) {
        return centered_square(cell, item, cell->y + ICON_BORDER);
    }
    int icon = preview_icon_size(item);
    return centered_square(cell, icon, cell->y + ICON_BORDER + item - icon);
}

wm_rect appearance_tabwin_text_rect(const wm_cycle_view *view, const wm_rect *cell,
                                    int label_height)
{
    int left = ROW_TEXT_SPACING + view->item_size + ROW_TEXT_SPACING;

    return (wm_rect) { cell->x + left, cell->y + (cell->height - label_height) / 2,
                       cell->width - left - ROW_TEXT_SPACING, label_height };
}

wm_rect appearance_tabwin_label_rect(wm_size size, int label_height)
{
    return (wm_rect) { TABWIN_INSET, size.height - TABWIN_INSET - label_height,
                       size.width - 2 * TABWIN_INSET, label_height };
}

size_t appearance_tabwin_item_count(const wm_cycle_view *view)
{
    return MIN(view->count, WM_CYCLE_ITEM_MAX);
}

static tabwin_grid grid_for(const wm_cycle_view *view, int label_height)
{
    int count = count_of(view);
    tabwin_grid grid = { .view = view, .label_height = label_height };

    if (view->mode == WM_TABWIN_LIST) {
        grid.rows = wm_clamp(view->rows, 1, count);
        grid.columns = ceil_div(count, grid.rows);
        grid.cell = list_cell(view, grid.columns, label_height);
    } else {
        grid.columns = wm_clamp(view->columns, 1, count);
        grid.rows = ceil_div(count, grid.columns);
        grid.cell = grid_cell(view, label_height);
    }
    return grid;
}

static wm_size panel_size(const tabwin_grid *grid)
{
    wm_size extent = grid_extent(grid);
    int label = grid->view->mode == WM_TABWIN_LIST ? 0 : LABEL_SPACING + grid->label_height;

    return (wm_size) { extent.width + 2 * TABWIN_INSET,
                       extent.height + label + 2 * TABWIN_INSET };
}

static wm_rect cell_rect(const tabwin_grid *grid, size_t index)
{
    int position = (int) index;
    bool list = grid->view->mode == WM_TABWIN_LIST;
    int column = list ? position / grid->rows : position % grid->columns;
    int row = list ? position % grid->rows : position / grid->columns;

    return (wm_rect) {
        TABWIN_INSET + column * (grid->cell.width + CELL_SPACING),
        TABWIN_INSET + row * (grid->cell.height + CELL_SPACING),
        grid->cell.width,
        grid->cell.height,
    };
}

static wm_rect preview_rect(const wm_cycle_view *view, const wm_rect *cell)
{
    int item = view->item_size;

    if (view->mode == WM_TABWIN_LIST || !view->preview) {
        return (wm_rect) { 0, 0, 0, 0 };
    }
    wm_rect square = centered_square(cell, item, cell->y + ICON_BORDER);
    square.height -= preview_icon_size(item);
    return square;
}

static int count_of(const wm_cycle_view *view)
{
    return wm_max((int) appearance_tabwin_item_count(view), 1);
}

static int ceil_div(int value, int divisor)
{
    return (value + divisor - 1) / divisor;
}

static wm_size grid_cell(const wm_cycle_view *view, int label_height)
{
    int size = view->item_size + label_height + 2 * ICON_BORDER;

    return (wm_size) { size, size };
}

static wm_size list_cell(const wm_cycle_view *view, int columns, int label_height)
{
    int content = wm_max(view->item_size, label_height);

    return (wm_size) { view->monitor.width / (columns + 1), content + 2 * ROW_PADDING };
}

static wm_size grid_extent(const tabwin_grid *grid)
{
    return (wm_size) {
        grid->columns * grid->cell.width + (grid->columns - 1) * CELL_SPACING,
        grid->rows * grid->cell.height + (grid->rows - 1) * CELL_SPACING,
    };
}

static wm_rect centered_square(const wm_rect *cell, int size, int top)
{
    return (wm_rect) { cell->x + (cell->width - size) / 2, top, size, size };
}

static int preview_icon_size(int item_size)
{
    return wm_min(item_size / PREVIEW_ICON_DIVISOR, PREVIEW_ICON_MAX);
}
