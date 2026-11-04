#include "menu_marks.h"

#include <math.h>

enum {
    INDICATOR_SIZE = 14,
};

static const double half_pixel = 0.5;
static const double line_width = 1.0;
static const double check_line_width = 2.0;
static const double dot_ratio = 0.25;
static const double arrow_ratio = 0.25;

typedef struct fraction_point {
    double x;
    double y;
} fraction_point;

static const fraction_point tick_points[] = { { 0.2, 0.5 }, { 0.425, 0.75 }, { 0.8, 0.25 } };

static void draw_box(cairo_t *cr, const wm_rect *box);
static void draw_circle(cairo_t *cr, const wm_rect *box);
static void draw_tick(cairo_t *cr, const wm_rect *box);
static void draw_dot(cairo_t *cr, const wm_rect *box);
static wm_rect indicator(const wm_rect *box);

void appearance_menu_mark_draw(cairo_t *cr, wm_menu_mark mark, const wm_rect *box)
{
    wm_rect inner = indicator(box);

    switch (mark) {
    case WM_MENU_MARK_CHECK_ON:
        draw_box(cr, &inner);
        draw_tick(cr, &inner);
        break;
    case WM_MENU_MARK_CHECK_OFF:
        draw_box(cr, &inner);
        break;
    case WM_MENU_MARK_RADIO_ON:
        draw_circle(cr, &inner);
        draw_dot(cr, &inner);
        break;
    case WM_MENU_MARK_RADIO_OFF:
        draw_circle(cr, &inner);
        break;
    default:
        break;
    }
}

void appearance_menu_arrow_draw(cairo_t *cr, const wm_rect *box)
{
    double half = box->height * arrow_ratio;
    double center_x = box->x + box->width / 2.0;
    double center_y = box->y + box->height / 2.0;

    cairo_move_to(cr, center_x - half / 2.0, center_y - half);
    cairo_line_to(cr, center_x + half / 2.0, center_y);
    cairo_line_to(cr, center_x - half / 2.0, center_y + half);
    cairo_close_path(cr);
    cairo_fill(cr);
}

static void draw_box(cairo_t *cr, const wm_rect *box)
{
    cairo_rectangle(cr, box->x + half_pixel, box->y + half_pixel, box->width - line_width,
                    box->height - line_width);
    cairo_set_line_width(cr, line_width);
    cairo_stroke(cr);
}

static void draw_circle(cairo_t *cr, const wm_rect *box)
{
    cairo_new_sub_path(cr);
    cairo_arc(cr, box->x + box->width / 2.0, box->y + box->height / 2.0,
              (box->width - line_width) / 2.0, 0.0, 2.0 * M_PI);
    cairo_set_line_width(cr, line_width);
    cairo_stroke(cr);
}

static void draw_tick(cairo_t *cr, const wm_rect *box)
{
    for (size_t i = 0; i < sizeof tick_points / sizeof tick_points[0]; i++) {
        cairo_line_to(cr, box->x + box->width * tick_points[i].x,
                      box->y + box->height * tick_points[i].y);
    }
    cairo_set_line_width(cr, check_line_width);
    cairo_stroke(cr);
}

static void draw_dot(cairo_t *cr, const wm_rect *box)
{
    cairo_new_sub_path(cr);
    cairo_arc(cr, box->x + box->width / 2.0, box->y + box->height / 2.0, box->width * dot_ratio,
              0.0, 2.0 * M_PI);
    cairo_fill(cr);
}

static wm_rect indicator(const wm_rect *box)
{
    return (wm_rect) { box->x + (box->width - INDICATOR_SIZE) / 2,
                       box->y + (box->height - INDICATOR_SIZE) / 2, INDICATOR_SIZE,
                       INDICATOR_SIZE };
}
