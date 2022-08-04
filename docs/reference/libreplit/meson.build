md_files = []

if get_option('docs')
	libreplit_toml = configure_file(
		input: 'libreplit.toml.in',
		output: 'libreplit.toml',
		configuration: toml_conf,
		install: true,
		install_dir: docs_dir / 'libreplit',
	)

	custom_target('libreplit-docs',
		input: [libreplit_toml, libreplit_gir[0]],
		output: 'libreplit',
		command: [
			gi_docgen,
			'generate',
			gi_docgen_common_args,
			'--add-include-path=@0@'.format(meson.current_build_dir() / '../../../libreplit'),
			'--config=@INPUT0@',
			'--output-dir=@OUTPUT@',
			'--content-dir=@0@'.format(meson.current_build_dir()),
			'--content-dir=@0@'.format(meson.current_source_dir()),
			'@INPUT1@',
		],
		depends: [],
		depend_files: [md_files],
		build_by_default: true,
		install: true,
		install_dir: docs_dir,
	)
endif
