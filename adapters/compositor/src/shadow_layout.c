#include "shadow_layout.h"
#include "wm/geometry.h"

typedef struct axis_split {
    int start[3];
    int length[3];
} axis_split;

static axis_split split_axis(int origin, int length, int corner);

shadow_pieces shadow_layout_compute(const wm_rect *rect, const shadow_geometry *image)
{
    int corner = image->corner;
    int dest_corner = wm_min(corner, wm_min(rect->width / 2, rect->height / 2));
    axis_split src_x = split_axis(0, image->size.width, corner);
    axis_split src_y = split_axis(0, image->size.height, corner);
    axis_split dst_x = split_axis(rect->x, rect->width, dest_corner);
    axis_split dst_y = split_axis(rect->y, rect->height, dest_corner);
    shadow_pieces result;
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) {
            shadow_piece *piece = &result.pieces[row * 3 + column];
            piece->source = (wm_rect){src_x.start[column], src_y.start[row],
                                      src_x.length[column], src_y.length[row]};
            piece->dest = (wm_rect){dst_x.start[column], dst_y.start[row],
                                    dst_x.length[column], dst_y.length[row]};
        }
    }
    return result;
}

static axis_split split_axis(int origin, int length, int corner)
{
    int middle = length - 2 * corner;
    if (middle < 0) {
        middle = 0;
    }
    axis_split split = {
        .start = {origin, origin + corner, origin + corner + middle},
        .length = {corner, middle, corner},
    };
    return split;
}
