fs = import('fs')

unit_test_dependencies = files([
  '../src/parsing.c',
  '../src/logging.c',

  'test_utils.c'
])

test_csv_parse = executable('test_csv_parse', files(['test_csv_parse.c']) + unit_test_dependencies)
test('test_csv_parse', test_csv_parse)

fs.copyfile('etc/test_csv_parse1.csv', 'test_csv_parse1.csv')
