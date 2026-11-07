compositor_test_inc = [
  include_directories('.'),
  include_directories('../src'),
  compositor_api_inc,
  core_inc,
]

compositor_pure_tests = {
  'frame_bands': ['../src/decor/frame_bands.c'],
  'shadow_layout': ['../src/decor/shadow_layout.c'],
  'zoom_math': ['../src/output/zoom_math.c'],
  'usable_area': ['../src/output/usable_area.c'],
  'cursor_names': ['../src/cursor/cursor_names.c'],
  'cursor_owner': ['../src/cursor/cursor_owner.c'],
  'brightness_curve': ['../src/output/brightness_curve.c'],
  'color_plan': ['../src/output/color_plan.c'],
  'display_policy': ['../src/output/display_policy.c'],
  'mode_choice': ['../src/output/mode_choice.c'],
  'device_setting': ['../src/input/device_setting.c'],
  'icon_pixels': ['../src/toplevel/icon_pixels.c'],
  'find_cursor_rings': ['../src/osd/find_cursor_rings.c'],
  'osd_geometry': ['../src/osd/osd_geometry.c'],
  'session_record': ['../src/session/session_record.c'],
  'session_book': ['../src/session/session_book.c', '../src/session/session_record.c'],
  'session_apply': ['../src/session/session_apply.c'],
  'present_rules': ['../src/render/present_rules.c'],
  'scalable_layout': ['../src/cursor/scalable_layout.c'],
  'xcursor_resize': ['../src/cursor/xcursor_resize.c'],
  'repaint_timing': ['../src/render/repaint_timing.c', '../src/render/present_rules.c'],
  'scale_guess': ['../src/output/scale_guess.c'],
}

foreach name, sources : compositor_pure_tests
  test(
    'compositor_' + name,
    executable(
      'test_compositor_' + name,
      ['test_' + name + '.c'] + sources,
      include_directories: compositor_test_inc,
      dependencies: [check_dep, wm_core_dep, compositor_m_dep],
    ),
  )
endforeach

test(
  'compositor_input_mapping',
  executable(
    'test_compositor_input_mapping',
    ['test_input_mapping.c', '../src/input/input_mapping.c'],
    include_directories: compositor_test_inc,
    c_args: compositor_c_args,
    dependencies: [check_dep, wm_core_dep, wlroots_dep, xkbcommon_dep],
  ),
)

test(
  'compositor_drag_actions',
  executable(
    'test_compositor_drag_actions',
    ['test_drag_actions.c', '../src/input/drag_actions.c'],
    include_directories: compositor_test_inc,
    dependencies: [check_dep, wayland_server_dep],
  ),
)

test(
  'compositor_layer_mapping',
  executable(
    'test_compositor_layer_mapping',
    ['test_layer_mapping.c', '../src/shell/layer_mapping.c', layer_shell_header],
    include_directories: compositor_test_inc,
    dependencies: [check_dep, wayland_server_dep],
  ),
)

test(
  'compositor_input_transform',
  executable(
    'test_compositor_input_transform',
    ['test_input_transform.c', '../src/input/input_transform.c'],
    include_directories: compositor_test_inc + [compositor_api_inc],
    c_args: compositor_c_args,
    dependencies: [check_dep, wlroots_dep, cairo_dep, compositor_m_dep],
  ),
)

test(
  'compositor_pointer_prefs_compare',
  executable(
    'test_compositor_pointer_prefs_compare',
    ['test_pointer_prefs_compare.c', '../src/input/pointer_prefs_compare.c'],
    include_directories: compositor_test_inc + [compositor_api_inc],
    dependencies: [check_dep, cairo_dep],
  ),
)

compositor_x11_tests = {
  'x11_gravity': ['../src/xwayland/x11_gravity.c'],
  'x11_motif': ['../src/xwayland/x11_motif.c'],
  'x11_pixmap_icon': ['../src/xwayland/x11_pixmap_icon.c'],
  'ewmh_payload': ['../src/xwayland/ewmh_payload.c'],
  'xsettings': ['../src/xwayland/xsettings_table.c', '../src/xwayland/xsettings_wire.c', '../src/xwayland/xresources.c'],
  'restart_policy': ['../src/server/restart_policy.c'],
  'utf8_copy': ['../src/util/utf8_copy.c'],
}

foreach name, sources : compositor_x11_tests
  test(
    'compositor_' + name,
    executable(
      'test_compositor_' + name,
      ['test_' + name + '.c'] + sources,
      include_directories: compositor_test_inc + [compositor_api_inc],
      dependencies: [check_dep, wm_core_dep, xcb_dep, glib_dep, cairo_dep],
    ),
  )
endforeach

test(
  'compositor_output_identity',
  executable(
    'test_compositor_output_identity',
    ['test_output_identity.c', '../src/output/output_identity.c'],
    include_directories: compositor_test_inc + [compositor_api_inc],
    dependencies: [check_dep, display_hash_dep],
  ),
)
