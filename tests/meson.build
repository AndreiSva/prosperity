fs = import('fs')

unit_test_dependencies = files([
  '../src/parsing.c',
  '../src/logging.c',
  '../src/wrapper.c',

  'test_utils.c'
])

test_csv_parse = executable('test_csv_parse', files(['test_csv_parse.c']) + unit_test_dependencies)
test('test_csv_parse', test_csv_parse)
fs.copyfile('etc/test_csv_parse.csv', 'test_csv_parse.csv')

test_csv_parse_unicode = executable('test_csv_parse_unicode', files(['test_csv_parse_unicode.c']) + unit_test_dependencies)
test('test_csv_parse_unicode', test_csv_parse_unicode)
fs.copyfile('etc/test_csv_parse_unicode.csv', 'test_csv_parse_unicode.csv')

test_csv_put = executable('test_csv_put', files(['test_csv_put.c']) + unit_test_dependencies)
test('test_csv_put', test_csv_put)
