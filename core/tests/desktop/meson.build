desktop_fixture = files('fake_ports.c', 'fake_input.c')

desktop_tests = [
  'map',
  'focus',
  'stack',
  'states',
  'workspaces',
  'moveresize',
  'frame',
  'cycle',
  'menu',
  'misc',
  'window_info',
  'requests',
  'window_actions',
  'pager',
  'restore',
]

foreach name : desktop_tests
  desktop_test = executable(
    'test_desktop_' + name,
    ['test_' + name + '.c'] + desktop_fixture,
    link_with: wm_core,
    include_directories: [core_inc],
    dependencies: [check_dep, m_dep],
  )
  test('desktop_' + name, desktop_test, suite: 'desktop')
endforeach
