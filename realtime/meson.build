realtime_helper_install_path = get_option('prefix') / get_option('libexecdir') / 'xfwm-wayland-realtime'
realtime_compositor_path = get_option('prefix') / get_option('bindir') / 'xfwm-wayland'

executable(
  'xfwm-wayland-realtime',
  'main.c',
  c_args: ['-DREALTIME_COMPOSITOR_PATH="@0@"'.format(realtime_compositor_path)],
  install: true,
  install_dir: get_option('libexecdir'),
)

if get_option('setcap')
  meson.add_install_script(find_program('sh'), files('setcap.sh'), realtime_helper_install_path)
endif
