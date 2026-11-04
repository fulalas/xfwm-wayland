wayland_scanner = find_program('wayland-scanner', native: true)

wlroots_dep = dependency('wlroots-0.20')
wayland_server_dep = dependency('wayland-server')
xkbcommon_dep = dependency('xkbcommon')
libinput_dep = dependency('libinput')
pixman_dep = dependency('pixman-1')
cairo_dep = dependency('cairo')
glib_dep = dependency('glib-2.0')
json_glib_dep = dependency('json-glib-1.0')
rsvg_dep = dependency('librsvg-2.0')
xcb_dep = dependency('xcb')
xcb_ewmh_dep = dependency('xcb-ewmh')
xcb_icccm_dep = dependency('xcb-icccm')
libdrm_dep = dependency('libdrm').partial_dependency(compile_args: true, includes: true)
libdrm_link_dep = dependency('libdrm')
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

input_device_list_header = custom_target(
  'xfce-input-device-list-private-v1-protocol.h',
  input: 'protocols/xfce-input-device-list-private-v1.xml',
  output: 'xfce-input-device-list-private-v1-protocol.h',
  command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

input_device_list_code = custom_target(
  'xfce-input-device-list-private-v1-protocol.c',
  input: 'protocols/xfce-input-device-list-private-v1.xml',
  output: 'xfce-input-device-list-private-v1-protocol.c',
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

toplevel_protocol_files = []
foreach protocol : [
  'xfce-foreign-toplevel-management-private-v1',
  'xfce-accessibility-private-v1',
  'wlr-foreign-toplevel-management-unstable-v1',
  'ext-workspace-v1',
]
  foreach kind : [['server-header', '-protocol.h'], ['private-code', '-protocol.c']]
    toplevel_protocol_files += custom_target(
      protocol + kind[1],
      input: 'protocols/' + protocol + '.xml',
      output: protocol + kind[1],
      command: [wayland_scanner, kind[0], '@INPUT@', '@OUTPUT@'],
    )
  endforeach
endforeach

present_protocol_dir = dependency('wayland-protocols').get_variable('pkgdatadir') / 'staging'
present_protocol_files = []
foreach protocol : [['fifo', 'fifo-v1'], ['commit-timing', 'commit-timing-v1']]
  foreach kind : [['server-header', '-protocol.h'], ['private-code', '-protocol.c']]
    present_protocol_files += custom_target(
      protocol[1] + kind[1],
      input: present_protocol_dir / protocol[0] / protocol[1] + '.xml',
      output: protocol[1] + kind[1],
      command: [wayland_scanner, kind[0], '@INPUT@', '@OUTPUT@'],
    )
  endforeach
endforeach

session_protocol_dir = dependency('wayland-protocols').get_variable('pkgdatadir')
session_protocol_files = []
foreach protocol : [
  ['staging/xdg-session-management', 'xdg-session-management-v1', ['server-header', 'private-code']],
  ['stable/xdg-shell', 'xdg-shell', ['private-code']],
]
  foreach kind : protocol[2]
    suffix = kind == 'server-header' ? '-protocol.h' : '-protocol.c'
    session_protocol_files += custom_target(
      protocol[1] + suffix,
      input: session_protocol_dir / protocol[0] / protocol[1] + '.xml',
      output: protocol[1] + suffix,
      command: [wayland_scanner, kind, '@INPUT@', '@OUTPUT@'],
    )
  endforeach
endforeach

display_protocol_files = []
foreach protocol : ['xfce-output-private-v1', 'xfce-output-management-private-v1']
  foreach kind : [['server-header', '-protocol.h'], ['private-code', '-protocol.c']]
    display_protocol_files += custom_target(
      protocol + kind[1],
      input: 'protocols/' + protocol + '.xml',
      output: protocol + kind[1],
      command: [wayland_scanner, kind[0], '@INPUT@', '@OUTPUT@'],
    )
  endforeach
endforeach

xwayland_protocol_dir = dependency('wayland-protocols').get_variable('pkgdatadir') / 'unstable'
xwayland_grab_files = []
foreach kind : [['server-header', '-protocol.h'], ['private-code', '-protocol.c']]
  xwayland_grab_files += custom_target(
    'xwayland-keyboard-grab-unstable-v1' + kind[1],
    input: xwayland_protocol_dir / 'xwayland-keyboard-grab' / 'xwayland-keyboard-grab-unstable-v1.xml',
    output: 'xwayland-keyboard-grab-unstable-v1' + kind[1],
    command: [wayland_scanner, kind[0], '@INPUT@', '@OUTPUT@'],
  )
endforeach

compositor_c_args = ['-DWLR_USE_UNSTABLE']

compositor_sources = files(
  'src/absolute_input.c',
  'src/accessibility.c',
  'src/activation.c',
  'src/brightness_curve.c',
  'src/cairo_buffer.c',
  'src/click_focus.c',
  'src/client_surfaces.c',
  'src/client_textures.c',
'src/clock.c',
  'src/commands.c',
  'src/commit_timing.c',
  'src/constraints.c',
  'src/color_plan.c',
  'src/color_preference.c',
  'src/cursor_image.c',
  'src/cursor_names.c',
  'src/cursor_owner.c',
  'src/cursor_sprite.c',
  'src/decorations.c',
  'src/device_list.c',
  'src/device_list_entry.c',
  'src/device_list_send.c',
  'src/device_setting.c',
  'src/device_settings.c',
  'src/device_settings_absolute.c',
  'src/device_settings_touchpad.c',
  'src/display_batch.c',
  'src/display_modules.c',
  'src/display_policy.c',
  'src/drag_actions.c',
  'src/displays.c',
  'src/drm_lease.c',
  'src/edid.c',
  'src/environment.c',
  'src/ewmh_payload.c',
  'src/ext_workspace_resource.c',
  'src/fifo.c',
  'src/fifo_surface.c',
  'src/find_cursor_paint.c',
  'src/find_cursor_rings.c',
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
  'src/input_transform.c',
  'src/keyboard.c',
  'src/keyboard_grabs.c',
  'src/keymap_cache.c',
  'src/layer_mapping.c',
  'src/layer_shell.c',
  'src/layer_surface.c',
  'src/libinput_calibration.c',
  'src/libinput_config.c',
  'src/libinput_methods.c',
  'src/lid_switch.c',
  'src/listener.c',
  'src/lock.c',
  'src/mode_choice.c',
  'src/node_owner.c',
  'src/osd.c',
  'src/osd_cycle.c',
  'src/osd_find_cursor.c',
  'src/osd_geometry.c',
  'src/osd_menu.c',
  'src/osd_outline.c',
  'src/osd_preview.c',
  'src/output.c',
  'src/output_arrange.c',
  'src/output_brightness.c',
  'src/output_color.c',
  'src/output_config.c',
  'src/output_config_apply.c',
  'src/output_frame.c',
  'src/output_head.c',
  'src/output_identity.c',
  'src/output_layout.c',
  'src/output_power.c',
  'src/output_record.c',
  'src/output_repaint.c',
  'src/output_setting.c',
  'src/paint_stats.c',
  'src/pointer.c',
  'src/pointer_prefs_compare.c',
  'src/pointer_target.c',
  'src/popup.c',
  'src/present_rules.c',
  'src/present_sync.c',
  'src/presenter.c',
  'src/protocols.c',
  'src/protocols_desktop.c',
  'src/protocols_input.c',
  'src/protocols_security.c',
  'src/render_sync.c',
  'src/renderer_switch.c',
  'src/repaint_timing.c',
  'src/resource.c',
'src/restart_policy.c',
  'src/scalable_cursor.c',
  'src/scalable_layout.c',
  'src/scale_guess.c',
  'src/scene_buffers.c',
'src/sealed_memfd.c',
  'src/scene_layers.c',
  'src/seat.c',
  'src/seat_resource.c',
  'src/selection.c',
  'src/server.c',
  'src/server_modules.c',
  'src/session_apply.c',
  'src/session_book.c',
  'src/session_file.c',
  'src/session_record.c',
  'src/shadow.c',
  'src/shadow_cache.c',
  'src/shadow_layout.c',
  'src/shortcuts.c',
  'src/tablet.c',
  'src/tablet_axes.c',
  'src/tablet_pad.c',
  'src/tablet_route.c',
  'src/tablet_tool.c',
  'src/timing_surface.c',
  'src/toplevel_capture.c',
  'src/toplevel_icons.c',
  'src/touch.c',
  'src/unmanaged.c',
  'src/usable_area.c',
'src/utf8_copy.c',
  'src/view.c',
  'src/view_commands.c',
  'src/view_present.c',
  'src/view_query.c',
  'src/virtual_input.c',
  'src/workspace_protocol.c',
  'src/x11_atoms.c',
  'src/x11_gravity.c',
  'src/x11_link.c',
  'src/xdg_decorations.c',
  'src/xdg_session.c',
  'src/xdg_session_restore.c',
  'src/xdg_session_toplevel.c',
  'src/xdg_shell.c',
  'src/xdg_view.c',
  'src/xdg_view_describe.c',
  'src/xdg_view_requests.c',
  'src/xfce_output_heads.c',
  'src/xfce_outputs.c',
  'src/xfce_toplevel.c',
  'src/xfce_toplevel_highlight.c',
  'src/xfce_toplevel_pixels.c',
  'src/xfce_toplevel_publish.c',
  'src/xfce_toplevel_requests.c',
  'src/xresources.c',
  'src/xsettings_table.c',
  'src/xsettings_wire.c',
  'src/xwayland.c',
  'src/xwayland_describe.c',
  'src/xwayland_ewmh.c',
  'src/xwayland_geometry.c',
  'src/xwayland_messages.c',
  'src/xwayland_properties.c',
  'src/xwayland_requests.c',
  'src/xwayland_restart.c',
  'src/xwayland_view.c',
  'src/xwayland_window_props.c',
  'src/xwayland_xsettings.c',
  'src/zoom.c',
  'src/zoom_math.c',
  'src/zoom_render.c',
)

compositor_lib = static_library(
  'compositor',
  compositor_sources,
  toplevel_protocol_files,
  layer_shell_header,
  output_power_header,
  present_protocol_files,
  session_protocol_files,
  display_protocol_files,
  input_device_list_header,
  input_device_list_code,
  xwayland_grab_files,
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
    json_glib_dep,
    rsvg_dep,
    xcb_dep,
    xcb_ewmh_dep,
    xcb_icccm_dep,
    libdrm_dep,
    libdrm_link_dep,
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
    json_glib_dep,
    rsvg_dep,
    xcb_dep,
    xcb_ewmh_dep,
    xcb_icccm_dep,
    libdrm_link_dep,
    compositor_m_dep,
  ],
)

subdir('tests')
