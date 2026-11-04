#ifndef COMPOSITOR_SEAT_PRIVATE_H
#define COMPOSITOR_SEAT_PRIVATE_H

#include <wayland-server-core.h>

#include "compositor/services.h"
#include "seat.h"

struct wlr_keyboard;
struct wlr_xcursor_manager;

typedef struct cursor_image cursor_image;
typedef struct keyboards keyboards;
typedef struct pointer_input pointer_input;
typedef struct touch_input touch_input;
typedef struct tablet_input tablet_input;
typedef struct absolute_input absolute_input;
typedef struct gestures gestures;
typedef struct constraints constraints;
typedef struct selection selection;
typedef struct virtual_input virtual_input;
typedef struct shortcuts shortcuts;

typedef struct button_input {
    uint32_t button;
    bool pressed;
    uint32_t time_ms;
} button_input;

typedef struct motion_delta {
    double dx;
    double dy;
} motion_delta;

typedef struct layout_point_fraction {
    double x;
    double y;
} layout_point_fraction;

typedef struct seat_focus {
    view *view;
    struct wlr_surface *layer;
    struct wlr_surface *unmanaged;
    struct wlr_surface *current;
} seat_focus;

struct seat {
    compositor *server;
    struct wlr_seat *wlr_seat;
    struct wlr_cursor *cursor;
    cursor_image *image;
    keyboards *keyboards;
    pointer_input *pointer;
    touch_input *touch;
    tablet_input *tablet;
    absolute_input *absolute;
    gestures *gestures;
    constraints *constraints;
    selection *selection;
    virtual_input *virtual_input;
    shortcuts *shortcuts;
    seat_focus focus;
    struct wl_list devices;
    struct wl_listener new_input;
};

void seat_track_device(seat *seat, struct wlr_input_device *device);

keyboards *keyboards_create(seat *seat);
void keyboards_destroy(keyboards *keyboards);
void keyboards_add(keyboards *keyboards, struct wlr_keyboard *keyboard);
void keyboards_add_virtual(keyboards *keyboards, struct wlr_keyboard *keyboard);
void keyboards_reload(keyboards *keyboards);
shortcuts *shortcuts_create(seat *seat);
void shortcuts_destroy(shortcuts *shortcuts);
void shortcuts_focus_changed(shortcuts *shortcuts, struct wlr_surface *surface);
bool shortcuts_inhibited(const shortcuts *shortcuts, const struct wlr_surface *surface);
void libinput_config_apply(struct wlr_input_device *device, const pointer_prefs *prefs);

pointer_input *pointer_input_create(seat *seat);
void pointer_input_destroy(pointer_input *pointer);
void pointer_input_motion(pointer_input *pointer, uint32_t time_ms);
void pointer_input_button(pointer_input *pointer, const button_input *input);
void pointer_input_surface_gone(pointer_input *pointer, struct wlr_surface *surface);

cursor_image *cursor_image_create(seat *seat);
void cursor_image_destroy(cursor_image *image);
void cursor_image_reload(cursor_image *image);
void cursor_image_set_core_shape(cursor_image *image, wm_cursor_shape shape);
void cursor_image_use_core(cursor_image *image);
void cursor_image_use_client(cursor_image *image);
struct wlr_xcursor_manager *cursor_image_xcursors(const cursor_image *image);

touch_input *touch_input_create(seat *seat);
void touch_input_destroy(touch_input *touch);
tablet_input *tablet_input_create(seat *seat);
void tablet_input_destroy(tablet_input *tablet);
gestures *gestures_create(seat *seat);
void gestures_destroy(gestures *gestures);
constraints *constraints_create(seat *seat);
void constraints_destroy(constraints *constraints);
motion_delta constraints_filter_motion(const constraints *constraints, motion_delta delta);
selection *selection_create(seat *seat);
void selection_destroy(selection *selection);
void selection_move_icons(selection *selection);
virtual_input *virtual_input_create(seat *seat);
void virtual_input_destroy(virtual_input *virtual_input);

#endif
