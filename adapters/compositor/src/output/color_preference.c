#include "color_preference.h"

#include <wlr/types/wlr_color_management_v1.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>

#include "render/client_surfaces.h"

static void refresh_surface(struct wlr_surface *surface, void *data);
static bool shown_on_hdr(const struct wlr_surface *surface);

void color_preference_refresh(compositor *c)
{
    if (c->globals.color_manager != NULL) {
        client_surfaces_for_each(c->display, refresh_surface, c->globals.color_manager);
    }
}

static void refresh_surface(struct wlr_surface *surface, void *data)
{
    bool hdr = shown_on_hdr(surface);
    struct wlr_image_description_v1_data preferred = {
        .tf_named = wlr_color_manager_v1_transfer_function_from_wlr(
            hdr ? WLR_COLOR_TRANSFER_FUNCTION_ST2084_PQ : WLR_COLOR_TRANSFER_FUNCTION_GAMMA22),
        .primaries_named = wlr_color_manager_v1_primaries_from_wlr(
            hdr ? WLR_COLOR_NAMED_PRIMARIES_BT2020 : WLR_COLOR_NAMED_PRIMARIES_SRGB),
    };
    wlr_color_manager_v1_set_surface_preferred_image_description(data, surface, &preferred);
}

static bool shown_on_hdr(const struct wlr_surface *surface)
{
    const struct wlr_surface_output *shown;
    wl_list_for_each(shown, &surface->current_outputs, link) {
        if (shown->output->image_description != NULL) {
            return true;
        }
    }
    return false;
}
