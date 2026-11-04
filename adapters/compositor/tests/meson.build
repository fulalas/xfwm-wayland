compositor_test_inc = [
  include_directories('.'),
  include_directories('../src'),
  compositor_api_inc,
  core_inc,
]

compositor_pure_tests = {
  'frame_bands': ['../src/frame_bands.c'],
  'shadow_layout': ['../src/shadow_layout.c'],
  'zoom_math': ['../src/zoom_math.c'],
  'usable_area': ['../src/usable_area.c'],
  'cursor_names': ['../src/cursor_names.c'],
  'cursor_owner': ['../src/cursor_owner.c'],
  'brightness_curve': ['../src/brightness_curve.c'],
  'color_plan': ['../src/color_plan.c'],
  'display_policy': ['../src/display_policy.c'],
  'mode_choice': ['../src/mode_choice.c'],
  'device_setting': ['../src/device_setting.c'],
  'icon_pixels': ['../src/icon_pixels.c'],
  'find_cursor_rings': ['../src/find_cursor_rings.c'],
  'osd_geometry': ['../src/osd_geometry.c'],
  'session_record': ['../src/session_record.c'],
  'session_book': ['../src/session_book.c', '../src/session_record.c'],
  'session_apply': ['../src/session_apply.c'],
  'present_rules': ['../src/present_rules.c'],
  'scalable_layout': ['../src/scalable_layout.c'],
  'repaint_timing': ['../src/repaint_timing.c', '../src/present_rules.c'],
  'scale_guess': ['../src/scale_guess.c'],
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
    ['test_input_mapping.c', '../src/input_mapping.c'],
    include_directories: compositor_test_inc,
    c_args: compositor_c_args,
    dependencies: [check_dep, wlroots_dep, xkbcommon_dep],
  ),
)

test(
  'compositor_drag_actions',
  executable(
    'test_compositor_drag_actions',
    ['test_drag_actions.c', '../src/drag_actions.c'],
    include_directories: compositor_test_inc,
    dependencies: [check_dep, wayland_server_dep],
  ),
)

test(
  'compositor_layer_mapping',
  executable(
    'test_compositor_layer_mapping',
    ['test_layer_mapping.c', '../src/layer_mapping.c', layer_shell_header],
    include_directories: compositor_test_inc,
    dependencies: [check_dep, wayland_server_dep],
  ),
)

test(
  'compositor_input_transform',
  executable(
    'test_compositor_input_transform',
    ['test_input_transform.c', '../src/input_transform.c'],
    include_directories: compositor_test_inc + [compositor_api_inc],
    c_args: compositor_c_args,
    dependencies: [check_dep, wlroots_dep, cairo_dep, compositor_m_dep],
  ),
)

test(
  'compositor_pointer_prefs_compare',
  executable(
    'test_compositor_pointer_prefs_compare',
    ['test_pointer_prefs_compare.c', '../src/pointer_prefs_compare.c'],
    include_directories: compositor_test_inc + [compositor_api_inc],
    dependencies: [check_dep, cairo_dep],
  ),
)

compositor_x11_tests = {
  'x11_gravity': ['../src/x11_gravity.c'],
  'ewmh_payload': ['../src/ewmh_payload.c'],
  'xsettings': ['../src/xsettings_table.c', '../src/xsettings_wire.c', '../src/xresources.c'],
  'restart_policy': ['../src/restart_policy.c'],
  'utf8_copy': ['../src/utf8_copy.c'],
}

foreach name, sources : compositor_x11_tests
  test(
    'compositor_' + name,
    executable(
      'test_compositor_' + name,
      ['test_' + name + '.c'] + sources,
      include_directories: compositor_test_inc + [compositor_api_inc],
      dependencies: [check_dep, wm_core_dep, xcb_dep],
    ),
  )
endforeach

test(
  'compositor_output_identity',
  executable(
    'test_compositor_output_identity',
    ['test_output_identity.c', '../src/output_identity.c'],
    include_directories: compositor_test_inc + [compositor_api_inc],
    dependencies: [check_dep, glib_dep],
  ),
)
