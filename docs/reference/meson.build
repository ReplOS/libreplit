toml_conf = configuration_data()
toml_conf.set('version', meson.project_version())

gi_docgen = find_program('gi-docgen', required: get_option('docs'))
gi_docgen_common_args = ['--quiet', '--no-namespace-dir']

docs_dir = replit_datadir / 'doc'

if get_option('docs') and not build_gir
	error('To build docs, make sure to enable introspection')
endif

subdir('libreplit')
