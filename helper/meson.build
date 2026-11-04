helper_gtk_dep = dependency('gtk+-3.0')
helper_gio_unix_dep = dependency('gio-unix-2.0')

executable(
  'xfwm-wayland-helper',
  files(
    'icon_resolver.c',
    'main.c',
    'protocol_io.c',
    'protocol_text.c',
    'style_report.c',
    'style_service.c',
    'terminate_dialog.c',
    'ui_style.c',
  ),
  dependencies: [helper_gtk_dep, helper_gio_unix_dep],
  install: true,
  install_dir: get_option('libexecdir'),
)
