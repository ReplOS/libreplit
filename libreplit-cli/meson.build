rquery_sources = [
	'main.c',
]

rquery_deps = [
	dependency('glib-2.0'),
]

if get_option('rquery')
	executable('rquery', rquery_sources,
		c_args: ['-Wno-missing-field-initializers'],
		dependencies: rquery_deps,
		install: true,
		link_with: [libreplit],
	)
endif
