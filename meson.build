project(
  'xfwm-wayland',
  'c',
  version: files('VERSION'),
  license: 'GPL-2.0-or-later',
  meson_version: '>=1.0',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'werror=true',
    'buildtype=release',
    'b_ndebug=if-release',
    'b_lto=true',
  ],
)

add_project_arguments('-D_GNU_SOURCE', language: 'c')

core_inc = include_directories('core/include')
compositor_api_inc = include_directories('adapters/compositor/include')

subdir('core')
subdir('adapters/appearance')
subdir('adapters/compositor')
subdir('adapters/xfce')
subdir('helper')
subdir('realtime')
subdir('app')
i18n = import('i18n')
subdir('po')
subdir('data')
