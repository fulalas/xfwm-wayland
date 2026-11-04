executable(
  'xfwm-wayland',
  'main.c',
  'app.c',
  'appearance_sync.c',
  'commands.c',
  'wayland_source.c',
  'session_environment.c',
  'session_bus.c',
  c_args: [
    '-DAPP_VERSION="@0@"'.format(meson.project_version()),
    '-DAPP_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  ],
  dependencies: [
    compositor_dep,
    appearance_dep,
    xfce_dep,
    wm_core_dep,
    dependency('glib-2.0'),
  ],
  install: true,
)
