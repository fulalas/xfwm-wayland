model_tests = {
  'geometry': [],
  'config': [],
  'keys': [],
  'shadow': [],
  'decor_extents': ['decor_fixture.c'],
  'decor_buttons': ['decor_fixture.c'],
  'decor_images': ['decor_fixture.c'],
  'decor_title': ['decor_fixture.c'],
  'decor_paint': ['decor_fixture.c'],
  'decor_parts': ['decor_fixture.c'],
}

foreach name, extra_sources : model_tests
  model_test = executable(
    'test_model_' + name,
    ['test_' + name + '.c'] + extra_sources,
    link_with: wm_core,
    include_directories: [core_inc],
    dependencies: [check_dep, m_dep],
  )
  test('model_' + name, model_test, suite: 'model')
endforeach
