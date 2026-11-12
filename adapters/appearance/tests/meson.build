appearance_fixture = static_library(
  'appearance_fixture',
  'fixture.c',
  include_directories: core_inc,
  dependencies: [dependency('gdk-pixbuf-2.0'), dependency('cairo')],
)

foreach name : ['themerc', 'xpm', 'color_symbols', 'theme_metrics', 'shadow', 'menu_labels',
                'painter', 'canvas', 'color', 'menu_layout', 'mysvgsnap']
  exe = executable(
    'test_appearance_' + name,
    'test_' + name + '.c',
    include_directories: appearance_src_inc,
    link_with: appearance_fixture,
    dependencies: [appearance_dep, check_dep],
  )
  test('appearance_' + name, exe)
endforeach
