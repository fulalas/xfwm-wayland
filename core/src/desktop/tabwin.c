#include "tabwin.h"

#include <math.h>

#include "desktop_state.h"
#include "effects.h"
#include "monitors.h"

enum {
    WIN_ICON_SIZE = 48,
    WIN_PREVIEW_SIZE = 6 * WIN_ICON_SIZE,
    LISTVIEW_WIN_ICON_SIZE = WIN_ICON_SIZE / 2,
    WIN_ICON_BORDER = 5,
    DEFAULT_LABEL_HEIGHT = 30,
    MIN_ICON_SIZE = 8,
};

static const double WIN_MAX_RATIO = 0.80;
static const char LABEL_SAMPLE[] = "Xy";

typedef struct grid_layout {
    int columns;
    int rows;
} grid_layout;

typedef struct grid_cell {
    int row;
    int column;
} grid_cell;

static int label_height(const wm_desktop *desktop);
static void compute_icon_grid(tabwin_grid *grid, size_t count, int label);
static int grid_columns(const tabwin_grid *grid, int size_request);
static int ceil_div(size_t count, int divisor);
static void compute_list(tabwin_grid *grid, size_t count);
static grid_layout layout_for(const tabwin_grid *grid, int count);
static grid_cell cell_of(const tabwin_grid *grid, grid_layout layout, int position);
static int position_of(const tabwin_grid *grid, grid_layout layout, grid_cell cell);
static grid_cell wrap_columns(grid_cell cell, grid_layout layout, int delta);
static grid_cell wrap_rows(grid_cell cell, grid_layout layout, int delta);
typedef struct missing_fix {
    grid_layout layout;
    grid_move move;
    int count;
} missing_fix;

static grid_cell fix_missing(const tabwin_grid *grid, grid_cell cell, const missing_fix *fix);
static grid_cell fix_horizontal(const tabwin_grid *grid, grid_cell cell, const missing_fix *fix);
static grid_cell fix_vertical(const tabwin_grid *grid, grid_cell cell, const missing_fix *fix);

tabwin_grid tabwin_compute(const wm_desktop *desktop, size_t count)
{
    tabwin_grid grid;
    grid.mode = desktop->config.cycle.tabwin_mode;
    grid.preview = desktop->config.cycle.preview && effects_enabled(desktop);
    if (!monitors_primary_full(desktop, &grid.monitor)) {
        grid.monitor = monitors_full_at(desktop, monitors_pointer(desktop));
    }
    if (grid.mode == WM_TABWIN_GRID) {
        compute_icon_grid(&grid, count, label_height(desktop));
    } else {
        compute_list(&grid, count);
    }
    return grid;
}

size_t tabwin_select_delta(const tabwin_grid *grid, size_t count, size_t current, grid_move move)
{
    if (count == 0) {
        return 0;
    }
    grid_layout layout = layout_for(grid, (int) count);
    grid_cell cell = cell_of(grid, layout, (int) current);
    cell = wrap_columns(cell, layout, move.columns);
    cell = wrap_rows(cell, layout, move.rows);
    if (position_of(grid, layout, cell) >= (int) count) {
        missing_fix fix = {layout, move, (int) count};
        cell = fix_missing(grid, cell, &fix);
    }
    int position = position_of(grid, layout, cell);
    return position >= 0 && position < (int) count ? (size_t) position : current;
}

static int label_height(const wm_desktop *desktop)
{
    const wm_environment *environment = &desktop->ports.environment;
    wm_size size = environment->measure_title(environment->self, LABEL_SAMPLE);
    return size.height > 0 ? size.height : DEFAULT_LABEL_HEIGHT;
}

static void compute_icon_grid(tabwin_grid *grid, size_t count, int label)
{
    int standard = WIN_ICON_SIZE;
    grid->item_size = grid->preview ? WIN_PREVIEW_SIZE : standard;
    int size_request = grid->item_size + label + 2 * WIN_ICON_BORDER;
    grid->columns = grid_columns(grid, size_request);
    grid->rows = ceil_div(count, grid->columns);
    while (size_request * grid->rows + label > grid->monitor.height * WIN_MAX_RATIO) {
        grid->item_size /= 2;
        if (grid->preview && grid->item_size <= standard) {
            grid->preview = false;
            grid->item_size = standard;
        }
        size_request = grid->item_size + label + 2 * WIN_ICON_BORDER;
        grid->columns = grid_columns(grid, size_request);
        grid->rows = ceil_div(count, grid->columns);
        if (grid->item_size < MIN_ICON_SIZE) {
            grid->item_size = MIN_ICON_SIZE;
            break;
        }
    }
}

static int grid_columns(const tabwin_grid *grid, int size_request)
{
    int columns = (int) floor(grid->monitor.width * WIN_MAX_RATIO / (double) size_request);
    return columns > 1 ? columns : 1;
}

static int ceil_div(size_t count, int divisor)
{
    return (int) ceil((double) count / (double) divisor);
}

static void compute_list(tabwin_grid *grid, size_t count)
{
    grid->preview = false;
    grid->item_size = LISTVIEW_WIN_ICON_SIZE;
    int rows = (int) floor(grid->monitor.height * WIN_MAX_RATIO
                           / (double) (grid->item_size + 2 * WIN_ICON_BORDER));
    grid->rows = rows > 1 ? rows : 1;
    grid->columns = ceil_div(count, grid->rows);
}

static grid_layout layout_for(const tabwin_grid *grid, int count)
{
    grid_layout layout;
    if (grid->mode == WM_TABWIN_GRID) {
        layout.columns = count < grid->columns ? count : grid->columns;
        layout.rows = (count - 1) / layout.columns + 1;
    } else {
        layout.rows = count < grid->rows ? count : grid->rows;
        layout.columns = (count - 1) / layout.rows + 1;
    }
    return layout;
}

static grid_cell cell_of(const tabwin_grid *grid, grid_layout layout, int position)
{
    if (grid->mode == WM_TABWIN_GRID) {
        return (grid_cell) {position / layout.columns, position % layout.columns};
    }
    return (grid_cell) {position % layout.rows, position / layout.rows};
}

static int position_of(const tabwin_grid *grid, grid_layout layout, grid_cell cell)
{
    if (grid->mode == WM_TABWIN_GRID) {
        return cell.column + cell.row * layout.columns;
    }
    return cell.row + cell.column * layout.rows;
}

static grid_cell wrap_columns(grid_cell cell, grid_layout layout, int delta)
{
    cell.column += delta;
    if (cell.column < 0) {
        cell.column = layout.columns - 1;
        cell.row = cell.row - 1 < 0 ? layout.rows - 1 : cell.row - 1;
    } else if (cell.column >= layout.columns) {
        cell.column = 0;
        cell.row = layout.rows > 1 ? cell.row + 1 : cell.row;
    }
    return cell;
}

static grid_cell wrap_rows(grid_cell cell, grid_layout layout, int delta)
{
    cell.row += delta;
    if (cell.row < 0) {
        cell.row = layout.rows - 1;
        cell.column = cell.column - 1 < 0 ? layout.columns - 1 : cell.column - 1;
    } else if (cell.row >= layout.rows) {
        cell.row = 0;
        cell.column++;
        if (cell.column >= layout.columns) {
            cell.column = layout.rows != 1 ? layout.columns - 1 : 0;
        }
    }
    return cell;
}

static grid_cell fix_missing(const tabwin_grid *grid, grid_cell cell, const missing_fix *fix)
{
    if (fix->move.columns != 0) {
        return fix_horizontal(grid, cell, fix);
    }
    return fix_vertical(grid, cell, fix);
}

static grid_cell fix_horizontal(const tabwin_grid *grid, grid_cell cell, const missing_fix *fix)
{
    const grid_layout *layout = &fix->layout;
    bool icons = grid->mode == WM_TABWIN_GRID;
    if (fix->move.columns > 0) {
        if (icons) {
            return (grid_cell) {0, 0};
        }
        return (grid_cell) {cell.row + 1 >= layout->rows ? 0 : cell.row + 1, 0};
    }
    if (icons) {
        return (grid_cell) {cell.row, (fix->count - 1) % layout->columns};
    }
    return (grid_cell) {(fix->count - 1) % layout->rows, layout->columns - 1};
}

static grid_cell fix_vertical(const tabwin_grid *grid, grid_cell cell, const missing_fix *fix)
{
    const grid_layout *layout = &fix->layout;
    bool icons = grid->mode == WM_TABWIN_GRID;
    if (fix->move.rows > 0) {
        return (grid_cell) {0, cell.column + 1 >= layout->columns ? 0 : cell.column + 1};
    }
    if (icons) {
        return (grid_cell) {cell.row - 1, cell.column};
    }
    return (grid_cell) {(fix->count - 1) % layout->rows, layout->columns - 1};
}
