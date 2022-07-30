api_version = '0.1'

replit_sources = [
  'replit.c',
]

replit_headers = [
  'replit.h',
]

version_split = meson.project_version().split('.')
MAJOR_VERSION = version_split[0]
MINOR_VERSION = version_split[1]
MICRO_VERSION = version_split[2]

version_conf = configuration_data()
version_conf.set('VERSION', meson.project_version())
version_conf.set('MAJOR_VERSION', MAJOR_VERSION)
version_conf.set('MINOR_VERSION', MINOR_VERSION)
version_conf.set('MICRO_VERSION', MICRO_VERSION)

configure_file(
  input: 'replit-version.h.in',
  output: 'replit-version.h',
  configuration: version_conf,
  install: true,
  install_dir: join_paths(get_option('includedir'), 'replit')
)

replit_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('libgraphqlparser', version: '>= 7.0'),
]

replit_lib = shared_library('replit-' + api_version,
  replit_sources,
  dependencies: replit_deps,
  install: true,
)

install_headers(replit_headers, subdir: 'replit')

pkg = import('pkgconfig')

pkg.generate(
  description: 'A shared library for ...',
    libraries: replit_lib,
         name: 'replit',
     filebase: 'replit-' + api_version,
      version: meson.project_version(),
      subdirs: 'replit',
     requires: 'glib-2.0',
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)
