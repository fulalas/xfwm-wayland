appearance_inc = include_directories('include')
appearance_src_inc = include_directories('src')

appearance_deps = [
  wm_core_dep,
  dependency('cairo'),
  dependency('pangocairo'),
  dependency('gdk-pixbuf-2.0'),
  dependency('gdk-3.0'),
  dependency('glib-2.0'),
  dependency('librsvg-2.0', version: '>= 2.52'),
  meson.get_compiler('c').find_library('m', required: false),
]

appearance_lib = static_library(
  'appearance',
  files(
    'src/appearance.c',
    'src/canvas.c',
    'src/color.c',
    'src/color_symbols.c',
    'src/fallback_style.c',
    'src/frame_painter.c',
    'src/menu_labels.c',
    'src/menu_layout.c',
    'src/menu_marks.c',
    'src/label_painter.c',
    'src/menu_painter.c',
    'src/mysvg.c',
    'src/mysvgaxis.c',
    'src/mysvgparse.c',
    'src/mysvgsnap.c',
    'src/painter.c',
    'src/request_memo.c',
    'src/shadow.c',
    'src/tabwin_layout.c',
    'src/tabwin_painter.c',
    'src/text.c',
    'src/theme.c',
    'src/theme_dir.c',
    'src/theme_image.c',
    'src/themerc.c',
    'src/title_text.c',
    'src/ui_text.c',
    'src/xpm.c',
    'src/xpm_entry.c',
    'src/xpm_reader.c',
  ),
  include_directories: [appearance_inc, appearance_src_inc, core_inc, compositor_api_inc],
  c_args: ['-DAPPEARANCE_TEXT_DOMAIN="@0@"'.format(meson.project_name())],
  dependencies: appearance_deps,
)

appearance_dep = declare_dependency(
  link_with: appearance_lib,
  include_directories: [appearance_inc, core_inc, compositor_api_inc],
  dependencies: appearance_deps,
)

subdir('tests')
