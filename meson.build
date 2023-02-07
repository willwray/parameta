project('parameta', 'cpp',
  default_options : ['cpp_std=c++20', 'warning_level=3'],
  version : '0.9'
)

parameta_dep = declare_dependency(
  include_directories : include_directories('.')
)

test('test parameta',
  executable('test_parameta', 'tests/test_parameta.cpp',
  dependencies : [parameta_dep])
)
