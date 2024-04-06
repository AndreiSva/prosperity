project('prosperity', 'c')
subdir('src')
openssl_dep = dependency('openssl')

executable('prosperity', server_sources, dependencies : [openssl_dep])

if get_option('tests') == true
  subdir('tests')
endif
