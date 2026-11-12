wayland_scanner = find_program('wayland-scanner', native: true)

pkg_config_env = environment()
if get_option('pkg_config_path').length() > 0
  pkg_config_env.prepend('PKG_CONFIG_PATH', get_option('pkg_config_path'))
endif
wlroots_name = ''
pkg_config_list = run_command(find_program('pkg-config', 'pkgconf'), '--list-all', env: pkg_config_env, check: true)
foreach line : pkg_config_list.stdout().split('\n')
  name = line.split(' ')[0]
  if name.startswith('wlroots-') and name.substring(8).version_compare('>=0.20')
    if wlroots_name == '' or name.substring(8).version_compare('>' + wlroots_name.substring(8))
      wlroots_name = name
    endif
  endif
endforeach
if wlroots_name == ''
  error('wlroots 0.20 or newer not found')
endif
wlroots_dep = dependency(wlroots_name)
wayland_server_dep = dependency('wayland-server')
xkbcommon_dep = dependency('xkbcommon')
libinput_dep = dependency('libinput')
pixman_dep = dependency('pixman-1')
cairo_dep = dependency('cairo')
glib_dep = dependency('glib-2.0')
json_glib_dep = dependency('json-glib-1.0')
rsvg_dep = dependency('librsvg-2.0', version: '>= 2.52')
xcb_dep = dependency('xcb')
xcb_ewmh_dep = dependency('xcb-ewmh')
xcb_icccm_dep = dependency('xcb-icccm')
xcb_shape_dep = dependency('xcb-shape')
libdrm_dep = dependency('libdrm').partial_dependency(compile_args: true, includes: true)
libdrm_link_dep = dependency('libdrm')
egl_dep = dependency('egl')
compositor_m_dep = meson.get_compiler('c').find_library('m', required: false)

display_hash_lib = static_library(
  'display_hash',
  'src/output/display_hash.c',
  include_directories: compositor_api_inc,
  dependencies: glib_dep,
)

display_hash_dep = declare_dependency(
  link_with: display_hash_lib,
  include_directories: compositor_api_inc,
  dependencies: glib_dep,
)

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
foreach protocol : [
  ['fifo', 'fifo-v1'],
  ['commit-timing', 'commit-timing-v1'],
  ['xdg-activation', 'xdg-activation-v1'],
]
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
  ['unstable/xdg-output', 'xdg-output-unstable-v1', ['server-header', 'private-code']],
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
  'src/input/absolute_input.c',
  'src/server/accessibility.c',
  'src/shell/activation.c',
  'src/shell/activation_protocol.c',
  'src/output/brightness_curve.c',
  'src/render/cairo_buffer.c',
  'src/input/click_focus.c',
  'src/render/client_surfaces.c',
  'src/render/client_textures.c',
'src/util/clock.c',
  'src/ports/commands.c',
  'src/render/commit_timing.c',
  'src/input/constraints.c',
  'src/output/color_plan.c',
  'src/output/color_preference.c',
  'src/input/cursor_image.c',
  'src/cursor/cursor_names.c',
  'src/cursor/cursor_owner.c',
  'src/cursor/cursor_sprite.c',
  'src/decor/decorations.c',
  'src/input/device_list.c',
  'src/input/device_list_entry.c',
  'src/input/device_list_send.c',
  'src/input/device_setting.c',
  'src/input/device_settings.c',
  'src/input/device_settings_absolute.c',
  'src/input/device_settings_touchpad.c',
  'src/output/display_batch.c',
  'src/output/display_policy.c',
  'src/input/drag_actions.c',
  'src/output/displays.c',
  'src/output/drm_lease.c',
  'src/output/edid.c',
  'src/ports/environment.c',
  'src/xwayland/ewmh_payload.c',
  'src/workspace/ext_workspace_resource.c',
  'src/render/fifo.c',
  'src/render/fifo_surface.c',
  'src/osd/find_cursor_paint.c',
  'src/osd/find_cursor_rings.c',
  'src/input/focus.c',
  'src/toplevel/foreign_toplevel.c',
  'src/toplevel/foreign_toplevel_requests.c',
  'src/toplevel/foreign_toplevel_sync.c',
  'src/decor/frame_bands.c',
  'src/input/gestures.c',
  'src/toplevel/icon_convert.c',
  'src/toplevel/icon_pixels.c',
  'src/toplevel/icon_slot.c',
  'src/input/idle.c',
  'src/input/input_mapping.c',
  'src/input/input_transform.c',
  'src/input/keyboard.c',
  'src/input/keyboard_grabs.c',
  'src/input/keymap_cache.c',
  'src/shell/layer_mapping.c',
  'src/shell/layer_shell.c',
  'src/shell/layer_surface.c',
  'src/input/libinput_calibration.c',
  'src/input/libinput_config.c',
  'src/input/libinput_methods.c',
  'src/input/lid_switch.c',
  'src/util/listener.c',
  'src/server/lock.c',
  'src/server/mapping_line.c',
  'src/server/memory_trim.c',
  'src/output/mode_choice.c',
  'src/util/node_owner.c',
  'src/osd/osd.c',
  'src/osd/osd_cycle.c',
  'src/osd/osd_find_cursor.c',
  'src/osd/osd_geometry.c',
  'src/osd/osd_menu.c',
  'src/osd/osd_outline.c',
  'src/osd/osd_size.c',
  'src/render/surface_opacity.c',
  'src/osd/osd_preview.c',
  'src/output/output.c',
  'src/output/output_arrange.c',
  'src/output/output_brightness.c',
  'src/output/output_color.c',
  'src/output/output_config.c',
  'src/output/output_config_apply.c',
  'src/output/output_frame.c',
  'src/output/output_head.c',
  'src/output/output_identity.c',
  'src/output/output_layout.c',
  'src/output/output_power.c',
  'src/output/output_record.c',
  'src/output/output_repaint.c',
  'src/output/output_setting.c',
  'src/render/paint_stats.c',
  'src/input/pointer.c',
  'src/input/pointer_prefs_compare.c',
  'src/input/pointer_target.c',
  'src/shell/popup.c',
  'src/render/present_rules.c',
  'src/render/present_sync.c',
  'src/render/presenter.c',
  'src/server/page_runs.c',
  'src/server/protocols.c',
  'src/server/protocols_desktop.c',
  'src/server/protocols_input.c',
  'src/server/protocols_security.c',
  'src/render/render_sync.c',
  'src/server/renderer_switch.c',
  'src/render/repaint_timing.c',
  'src/util/resource.c',
'src/server/restart_policy.c',
  'src/cursor/scalable_cursor.c',
  'src/cursor/scalable_layout.c',
  'src/output/scale_guess.c',
  'src/render/scene_buffers.c',
'src/util/sealed_memfd.c',
  'src/render/scene_layers.c',
  'src/input/seat.c',
  'src/input/seat_resource.c',
  'src/input/selection.c',
  'src/server/server.c',
  'src/server/server_modules.c',
  'src/session/session_apply.c',
  'src/session/session_book.c',
  'src/session/session_file.c',
  'src/session/session_record.c',
  'src/decor/shadow.c',
  'src/decor/shadow_cache.c',
  'src/decor/shadow_layout.c',
  'src/input/shortcuts.c',
  'src/input/tablet.c',
  'src/input/tablet_axes.c',
  'src/input/tablet_pad.c',
  'src/input/tablet_route.c',
  'src/input/tablet_tool.c',
  'src/render/timing_surface.c',
  'src/toplevel/toplevel_capture.c',
  'src/toplevel/toplevel_icons.c',
  'src/input/touch.c',
  'src/xwayland/unmanaged.c',
  'src/output/usable_area.c',
'src/util/utf8_copy.c',
  'src/shell/view.c',
  'src/shell/view_commands.c',
  'src/shell/view_present.c',
  'src/shell/view_query.c',
  'src/input/virtual_input.c',
  'src/toplevel/window_capture_source.c',
  'src/workspace/workspace_protocol.c',
  'src/xwayland/x11_atoms.c',
  'src/xwayland/x11_gravity.c',
  'src/xwayland/x11_link.c',
  'src/xwayland/x11_motif.c',
  'src/xwayland/x11_pixmap_icon.c',
  'src/xwayland/x11_scale.c',
  'src/render/scaled_surface.c',
  'src/output/xdg_output.c',
  'src/cursor/xcursor_cache.c',
  'src/cursor/xcursor_file.c',
  'src/cursor/xcursor_resize.c',
  'src/shell/xdg_decorations.c',
  'src/session/xdg_session.c',
  'src/session/xdg_session_restore.c',
  'src/session/xdg_session_toplevel.c',
  'src/shell/xdg_shell.c',
  'src/shell/xdg_view.c',
  'src/shell/xdg_view_describe.c',
  'src/shell/xdg_view_requests.c',
  'src/output/xfce_output_heads.c',
  'src/output/xfce_outputs.c',
  'src/toplevel/xfce_toplevel.c',
  'src/toplevel/xfce_toplevel_highlight.c',
  'src/toplevel/xfce_toplevel_pixels.c',
  'src/toplevel/xfce_toplevel_publish.c',
  'src/toplevel/xfce_toplevel_requests.c',
  'src/xwayland/xresources.c',
  'src/xwayland/xsettings_table.c',
  'src/xwayland/xsettings_wire.c',
  'src/xwayland/xwayland.c',
  'src/xwayland/xwayland_describe.c',
  'src/xwayland/xwayland_ewmh.c',
  'src/xwayland/xwayland_geometry.c',
  'src/xwayland/xwayland_keys.c',
  'src/xwayland/xwayland_messages.c',
  'src/xwayland/xwayland_properties.c',
  'src/xwayland/xwayland_props.c',
  'src/xwayland/xwayland_requests.c',
  'src/xwayland/xwayland_restart.c',
  'src/xwayland/xwayland_view.c',
  'src/xwayland/xwayland_window_props.c',
  'src/xwayland/xwayland_xsettings.c',
  'src/output/zoom.c',
  'src/output/zoom_math.c',
  'src/output/zoom_render.c',
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
    display_hash_dep,
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
    xcb_shape_dep,
    libdrm_dep,
    libdrm_link_dep,
    egl_dep,
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
    xcb_shape_dep,
    libdrm_link_dep,
    egl_dep,
    compositor_m_dep,
  ],
)

subdir('tests')
