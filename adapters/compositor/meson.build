wayland_scanner = find_program('wayland-scanner', native: true)

wlroots_dep = dependency('wlroots-0.20')
wayland_server_dep = dependency('wayland-server')
xkbcommon_dep = dependency('xkbcommon')
libinput_dep = dependency('libinput')
pixman_dep = dependency('pixman-1')
cairo_dep = dependency('cairo')
glib_dep = dependency('glib-2.0')
xcb_dep = dependency('xcb')
xcb_ewmh_dep = dependency('xcb-ewmh')
xcb_icccm_dep = dependency('xcb-icccm')
libdrm_dep = dependency('libdrm').partial_dependency(compile_args: true, includes: true)
compositor_m_dep = meson.get_compiler('c').find_library('m', required: false)

layer_shell_header = custom_target(
  'wlr-layer-shell-unstable-v1-protocol.h',
  input: 'protocols/wlr-layer-shell-unstable-v1.xml',
  output: 'wlr-layer-shell-unstable-v1-protocol.h',
  command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

output_power_header = custom_target(
  'wlr-output-power-management-unstable-v1-protocol.h',
  input: 'protocols/wlr-output-power-management-unstable-v1.xml',
  output: 'wlr-output-power-management-unstable-v1-protocol.h',
  command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

compositor_c_args = ['-DWLR_USE_UNSTABLE']

compositor_sources = files(
  'src/activation.c',
  'src/cairo_buffer.c',
  'src/commands.c',
  'src/constraints.c',
  'src/cursor_image.c',
  'src/cursor_names.c',
  'src/decorations.c',
  'src/environment.c',
  'src/focus.c',
  'src/foreign_toplevel.c',
  'src/foreign_toplevel_requests.c',
  'src/foreign_toplevel_sync.c',
  'src/frame_bands.c',
  'src/gestures.c',
  'src/icon_convert.c',
  'src/icon_pixels.c',
  'src/icon_slot.c',
  'src/idle.c',
  'src/input_mapping.c',
  'src/keyboard.c',
  'src/keymap_cache.c',
  'src/layer_mapping.c',
  'src/layer_shell.c',
  'src/layer_surface.c',
  'src/libinput_config.c',
  'src/listener.c',
  'src/lock.c',
  'src/node_owner.c',
  'src/osd.c',
  'src/osd_cycle.c',
  'src/osd_geometry.c',
  'src/osd_menu.c',
  'src/osd_outline.c',
  'src/output.c',
  'src/output_arrange.c',
  'src/output_config.c',
  'src/output_config_apply.c',
  'src/output_frame.c',
  'src/output_layout.c',
  'src/output_power.c',
  'src/paint_stats.c',
  'src/pointer.c',
  'src/pointer_target.c',
  'src/popup.c',
  'src/presenter.c',
  'src/protocols.c',
  'src/protocols_desktop.c',
  'src/protocols_input.c',
  'src/protocols_security.c',
  'src/scene_buffers.c',
  'src/scene_layers.c',
  'src/seat.c',
  'src/selection.c',
  'src/server.c',
  'src/server_modules.c',
  'src/shadow.c',
  'src/shadow_cache.c',
  'src/shadow_layout.c',
  'src/shortcuts.c',
  'src/tablet.c',
  'src/toplevel_capture.c',
  'src/toplevel_icons.c',
  'src/touch.c',
  'src/unmanaged.c',
  'src/usable_area.c',
  'src/view.c',
  'src/view_commands.c',
  'src/view_present.c',
  'src/view_query.c',
  'src/virtual_input.c',
  'src/workspace_protocol.c',
  'src/xdg_decorations.c',
  'src/xdg_shell.c',
  'src/xdg_view.c',
  'src/xdg_view_describe.c',
  'src/xdg_view_requests.c',
  'src/xwayland.c',
  'src/xwayland_describe.c',
  'src/xwayland_properties.c',
  'src/xwayland_requests.c',
  'src/xwayland_view.c',
  'src/zoom.c',
  'src/zoom_math.c',
  'src/zoom_render.c',
)

compositor_lib = static_library(
  'compositor',
  compositor_sources,
  layer_shell_header,
  output_power_header,
  include_directories: [compositor_api_inc, core_inc, include_directories('src')],
  c_args: compositor_c_args,
  dependencies: [
    wm_core_dep,
    wlroots_dep,
    wayland_server_dep,
    xkbcommon_dep,
    libinput_dep,
    pixman_dep,
    cairo_dep,
    glib_dep,
    xcb_dep,
    xcb_ewmh_dep,
    xcb_icccm_dep,
    libdrm_dep,
    compositor_m_dep,
  ],
)

compositor_dep = declare_dependency(
  link_with: compositor_lib,
  include_directories: [include_directories('include'), core_inc],
  dependencies: [
    wm_core_dep,
    wlroots_dep,
    wayland_server_dep,
    xkbcommon_dep,
    libinput_dep,
    pixman_dep,
    cairo_dep,
    xcb_dep,
    xcb_ewmh_dep,
    xcb_icccm_dep,
    compositor_m_dep,
  ],
)

subdir('tests')
