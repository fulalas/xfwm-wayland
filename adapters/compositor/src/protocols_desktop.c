#include <wlr/types/wlr_ext_foreign_toplevel_list_v1.h>
#include <wlr/types/wlr_ext_image_capture_source_v1.h>
#include <wlr/types/wlr_ext_image_copy_capture_v1.h>
#include <wlr/types/wlr_ext_workspace_v1.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_output_power_management_v1.h>
#include <wlr/util/log.h>

#include "protocols.h"

enum {
    STAGING_VERSION = 1,
};

static void create_capture_sources(compositor *server);

bool protocols_create_desktop(compositor *server)
{
    struct wl_display *display = server->display;
    protocol_globals *globals = &server->globals;
    globals->foreign_toplevel = wlr_foreign_toplevel_manager_v1_create(display);
    globals->foreign_toplevel_list =
        wlr_ext_foreign_toplevel_list_v1_create(display, STAGING_VERSION);
    globals->workspace = wlr_ext_workspace_manager_v1_create(display, STAGING_VERSION);
    globals->output_manager = wlr_output_manager_v1_create(display);
    globals->output_power = wlr_output_power_manager_v1_create(display);
    create_capture_sources(server);
    bool created = globals->foreign_toplevel != NULL && globals->foreign_toplevel_list != NULL &&
                   globals->workspace != NULL && globals->output_manager != NULL &&
                   globals->output_power != NULL;
    if (!created) {
        wlr_log(WLR_ERROR, "cannot create the desktop integration globals");
    }
    return created;
}

static void create_capture_sources(compositor *server)
{
    struct wl_display *display = server->display;
    if (wlr_ext_output_image_capture_source_manager_v1_create(display, STAGING_VERSION) == NULL ||
        wlr_ext_image_copy_capture_manager_v1_create(display, STAGING_VERSION) == NULL) {
        wlr_log(WLR_ERROR, "cannot create the screen capture globals");
    }
    server->globals.toplevel_capture =
        wlr_ext_foreign_toplevel_image_capture_source_manager_v1_create(display, STAGING_VERSION);
}
