project('parameta', 'cpp',
  default_options : ['cpp_std=c++20', 'warning_level=3'],
  version : '0.5'
)

lml_namespace_dep = subproject('lml_namespace').get_variable('lml_namespace_dep')

parameta_dep = declare_dependency(
  dependencies: [lml_namespace_dep],
  include_directories : include_directories('.')
)

test('test parameta',
  executable('test_parameta', 'tests/test_parameta.cpp',
  dependencies : [parameta_dep])
)
