rquery_sources = [
	'main.c',
]

if get_option('rquery')
	rquery_deps = [
  	dependency('gio-2.0', version: '>= 2.50'),
		dependency('glib-2.0'),
		dependency('json-glib-1.0', version: '>= 1.6'),
		dependency('libsoup-3.0', version: '>= 3.0'),
		dependency('libreplit-0.1'),
	]

	executable('rquery', rquery_sources,
		c_args: ['-Wno-missing-field-initializers'],
		dependencies: rquery_deps,
		install: true,
	)
endif
