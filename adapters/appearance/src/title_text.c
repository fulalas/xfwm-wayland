#include "title_text.h"

static const wm_point frame_shadow_offsets[] = { { -1, 0 }, { 0, -1 }, { 1, 0 }, { 0, 1 } };
static const wm_point under_shadow_offset = { 1, 1 };

static void draw_shadow(cairo_t *cr, PangoLayout *layout, wm_title_shadow shadow);
static void show_at(cairo_t *cr, PangoLayout *layout, wm_point offset);
static int state_index(wm_frame_state state);

wm_size appearance_title_measure(const title_style *style, const char *title)
{
    PangoLayout *layout = appearance_text_layout(&style->font, title);
    wm_size size = appearance_text_layout_size(layout);

    g_object_unref(layout);
    return size;
}

void appearance_title_draw(cairo_t *cr, const title_style *style,
                           const frame_paint_request *request)
{
    const wm_text_op *op = &request->layout->title_text;
    PangoLayout *layout = appearance_text_layout(&style->font, request->title);
    int state = state_index(op->state);

    cairo_save(cr);
    cairo_rectangle(cr, op->clip.x, op->clip.y, op->clip.width, op->clip.height);
    cairo_clip(cr);
    cairo_translate(cr, op->position.x, op->position.y);
    if (op->shadow != WM_TITLE_SHADOW_NONE) {
        appearance_text_set_color(cr, &style->shadow[state]);
        draw_shadow(cr, layout, op->shadow);
    }
    appearance_text_set_color(cr, &style->text[state]);
    pango_cairo_show_layout(cr, layout);
    cairo_restore(cr);
    g_object_unref(layout);
}

static void draw_shadow(cairo_t *cr, PangoLayout *layout, wm_title_shadow shadow)
{
    if (shadow == WM_TITLE_SHADOW_UNDER) {
        show_at(cr, layout, under_shadow_offset);
        return;
    }
    for (size_t i = 0; i < G_N_ELEMENTS(frame_shadow_offsets); i++) {
        show_at(cr, layout, frame_shadow_offsets[i]);
    }
}

static void show_at(cairo_t *cr, PangoLayout *layout, wm_point offset)
{
    cairo_save(cr);
    cairo_translate(cr, offset.x, offset.y);
    pango_cairo_show_layout(cr, layout);
    cairo_restore(cr);
}

static int state_index(wm_frame_state state)
{
    return state == WM_FRAME_INACTIVE ? WM_FRAME_INACTIVE : WM_FRAME_ACTIVE;
}
