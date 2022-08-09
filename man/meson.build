if get_option('man')
	a2x = find_program('a2x')

	custom_target('man page',
		input: ['rquery.txt', 'stylesheet.xsl'],
		output: 'rquery.1',
		command: [
			a2x,
			'-D', '@OUTDIR@',
			'--xsl-file', '@INPUT1@',
			'-f', 'manpage',
			'@INPUT0@',
		],
		install_dir: replit_mandir + '/man1',
		install: true,
	)
endif
