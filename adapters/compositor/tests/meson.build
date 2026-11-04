compositor_test_inc = [
  include_directories('.'),
  include_directories('../src'),
  core_inc,
]

compositor_pure_tests = {
  'frame_bands': ['../src/frame_bands.c'],
  'shadow_layout': ['../src/shadow_layout.c'],
  'zoom_math': ['../src/zoom_math.c'],
  'usable_area': ['../src/usable_area.c'],
  'cursor_names': ['../src/cursor_names.c'],
  'icon_pixels': ['../src/icon_pixels.c'],
  'osd_geometry': ['../src/osd_geometry.c'],
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
  'compositor_layer_mapping',
  executable(
    'test_compositor_layer_mapping',
    ['test_layer_mapping.c', '../src/layer_mapping.c', layer_shell_header],
    include_directories: compositor_test_inc,
    dependencies: [check_dep, wayland_server_dep],
  ),
)
