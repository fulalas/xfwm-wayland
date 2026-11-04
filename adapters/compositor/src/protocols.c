#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_alpha_modifier_v1.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_content_type_v1.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_export_dmabuf_v1.h>
#include <wlr/types/wlr_ext_data_control_v1.h>
#include <wlr/types/wlr_fractional_scale_v1.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_linux_dmabuf_v1.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_security_context_v1.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_single_pixel_buffer_v1.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_tearing_control_v1.h>
#include <wlr/types/wlr_viewporter.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_dialog_v1.h>
#include <wlr/types/wlr_xdg_foreign_registry.h>
#include <wlr/types/wlr_xdg_foreign_v1.h>
#include <wlr/types/wlr_xdg_foreign_v2.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_system_bell_v1.h>
#include <wlr/types/wlr_xdg_toplevel_icon_v1.h>
#include <wlr/util/log.h>

#include "protocols.h"

enum {
    COMPOSITOR_VERSION = 6,
    LINUX_DMABUF_VERSION = 4,
    PRESENTATION_VERSION = 2,
    STAGING_VERSION = 1,
};

static bool create_core(compositor *server);
static void create_dmabuf(compositor *server);
static void create_capture(compositor *server);
static void create_clipboard(compositor *server);
static void create_shell_extras(compositor *server);
static void create_foreign(compositor *server);

bool protocols_create(compositor *server)
{
    if (!create_core(server)) {
        wlr_log(WLR_ERROR, "cannot create the core Wayland globals");
        return false;
    }
    create_dmabuf(server);
    create_capture(server);
    create_clipboard(server);
    create_shell_extras(server);
    create_foreign(server);
    return true;
}

static bool create_core(compositor *server)
{
    struct wl_display *display = server->display;
    server->globals.compositor =
        wlr_compositor_create(display, COMPOSITOR_VERSION, server->renderer);
    bool shm = wlr_renderer_init_wl_shm(server->renderer, display);
    wlr_subcompositor_create(display);
    wlr_data_device_manager_create(display);
    wlr_viewporter_create(display);
    wlr_single_pixel_buffer_manager_v1_create(display);
    wlr_fractional_scale_manager_v1_create(display, STAGING_VERSION);
    wlr_presentation_create(display, server->backend, PRESENTATION_VERSION);
    wlr_alpha_modifier_v1_create(display);
    wlr_content_type_manager_v1_create(display, STAGING_VERSION);
    return server->globals.compositor != NULL && shm;
}

static void create_dmabuf(compositor *server)
{
    if (wlr_renderer_get_texture_formats(server->renderer, WLR_BUFFER_CAP_DMABUF) == NULL) {
        return;
    }
    struct wlr_linux_dmabuf_v1 *dmabuf = wlr_linux_dmabuf_v1_create_with_renderer(
        server->display, LINUX_DMABUF_VERSION, server->renderer);
    if (dmabuf != NULL) {
        wlr_scene_set_linux_dmabuf_v1(server->scene, dmabuf);
    }
}

static void create_capture(compositor *server)
{
    struct wl_display *display = server->display;
    wlr_xdg_output_manager_v1_create(display, server->layout);
    wlr_screencopy_manager_v1_create(display);
    wlr_export_dmabuf_manager_v1_create(display);
    struct wlr_gamma_control_manager_v1 *gamma = wlr_gamma_control_manager_v1_create(display);
    if (gamma != NULL) {
        wlr_scene_set_gamma_control_manager_v1(server->scene, gamma);
    }
}

static void create_clipboard(compositor *server)
{
    struct wl_display *display = server->display;
    wlr_primary_selection_v1_device_manager_create(display);
    wlr_data_control_manager_v1_create(display);
    wlr_ext_data_control_manager_v1_create(display, STAGING_VERSION);
}

static void create_shell_extras(compositor *server)
{
    struct wl_display *display = server->display;
    protocol_globals *globals = &server->globals;
    globals->dialog = wlr_xdg_wm_dialog_v1_create(display, STAGING_VERSION);
    globals->toplevel_icon = wlr_xdg_toplevel_icon_manager_v1_create(display, STAGING_VERSION);
    globals->xdg_decoration = wlr_xdg_decoration_manager_v1_create(display);
    globals->server_decoration = wlr_server_decoration_manager_create(display);
    if (globals->server_decoration != NULL) {
        wlr_server_decoration_manager_set_default_mode(globals->server_decoration,
                                                       WLR_SERVER_DECORATION_MANAGER_MODE_SERVER);
    }
    globals->tearing = wlr_tearing_control_manager_v1_create(display, STAGING_VERSION);
    globals->security_context = wlr_security_context_manager_v1_create(display);
    wlr_xdg_system_bell_v1_create(display, STAGING_VERSION);
}

static void create_foreign(compositor *server)
{
    struct wlr_xdg_foreign_registry *registry = wlr_xdg_foreign_registry_create(server->display);
    if (registry == NULL) {
        return;
    }
    wlr_xdg_foreign_v1_create(server->display, registry);
    wlr_xdg_foreign_v2_create(server->display, registry);
}
