#include <string.h>
#include <wlr/types/wlr_security_context_v1.h>

#include "protocols.h"

static const char *const privileged_interfaces[] = {
    "zwlr_screencopy_manager_v1",
    "zwlr_export_dmabuf_manager_v1",
    "zwlr_gamma_control_manager_v1",
    "zwlr_data_control_manager_v1",
    "ext_data_control_manager_v1",
    "zwp_virtual_keyboard_manager_v1",
    "zwlr_virtual_pointer_manager_v1",
    "zwlr_layer_shell_v1",
    "ext_session_lock_manager_v1",
    "zwp_input_method_manager_v2",
    "zwp_keyboard_shortcuts_inhibit_manager_v1",
    "wp_security_context_manager_v1",
    "zwlr_foreign_toplevel_manager_v1",
    "ext_foreign_toplevel_list_v1",
    "ext_workspace_manager_v1",
    "zwlr_output_manager_v1",
    "zwlr_output_power_manager_v1",
    "ext_output_image_capture_source_manager_v1",
    "ext_foreign_toplevel_image_capture_source_manager_v1",
    "ext_image_copy_capture_manager_v1",
};

static bool filter_global(const struct wl_client *client, const struct wl_global *global,
                          void *data);
static bool is_privileged(const char *interface);

void protocols_restrict_sandboxed(compositor *server)
{
    wl_display_set_global_filter(server->display, filter_global, server);
}

static bool filter_global(const struct wl_client *client, const struct wl_global *global,
                          void *data)
{
    compositor *server = data;
    if (server->globals.security_context == NULL) {
        return true;
    }
    if (wlr_security_context_manager_v1_lookup_client(server->globals.security_context, client) ==
        NULL) {
        return true;
    }
    return !is_privileged(wl_global_get_interface(global)->name);
}

static bool is_privileged(const char *interface)
{
    size_t count = sizeof(privileged_interfaces) / sizeof(privileged_interfaces[0]);
    for (size_t i = 0; i < count; i++) {
        if (strcmp(interface, privileged_interfaces[i]) == 0) {
            return true;
        }
    }
    return false;
}
