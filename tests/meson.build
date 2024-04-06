fs = import('fs')

unit_test_dependencies = files([
  '../src/parsing.c',
  '../src/logging.c',

  'test_utils.c'
])

test_csv_parse = executable('test_csv_parse', files(['test_csv_parse.c']) + unit_test_dependencies)
test('test_csv_parse', test_csv_parse)
fs.copyfile('etc/test_csv_parse.csv', 'test_csv_parse.csv')

test_csv_parse_unicode = executable('test_csv_parse_unicode', files(['test_csv_parse_unicde.c']) + unit_test_dependencies)
test('test_csv_parse_unicode', test_csv_parse_unicode)
fs.copyfile('etc/test_csv_parse_unicde.csv', 'test_csv_parse_unicode.csv')
