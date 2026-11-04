m_dep = meson.get_compiler('c').find_library('m', required: false)

subdir('src/model')
subdir('src/desktop')

wm_core = static_library(
  'wm_core',
  model_sources + desktop_sources,
  include_directories: [core_inc, include_directories('src')],
  dependencies: m_dep,
  c_args: ['-Wpedantic'],
)

wm_core_dep = declare_dependency(
  link_with: wm_core,
  include_directories: core_inc,
  dependencies: m_dep,
)

check_lib = static_library(
  'check',
  'tests/check.c',
  include_directories: include_directories('tests'),
  dependencies: m_dep,
)

check_dep = declare_dependency(
  link_with: check_lib,
  include_directories: include_directories('tests'),
)

subdir('tests/model')
subdir('tests/desktop')
