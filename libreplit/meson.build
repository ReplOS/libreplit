api_version = '0.1'

replit_sources = [
  'replit-client.c',
  'replit-subscriber.c',
  'replit.c',
]

replit_headers = [
  'replit-client.h',
  'replit-subscriber.h',
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
  install_dir: join_paths(get_option('includedir'), 'libreplit')
)

replit_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('json-glib-1.0', version: '>= 1.6'),
  dependency('libsoup-3.0', version: '>= 3.0'),
]

libreplit = shared_library('replit-' + api_version,
  replit_sources,
  c_args: ['-DREPLIT_COMPILATION'],
  dependencies: replit_deps,
  install: true,
)

install_headers(replit_headers, subdir: 'libreplit')

gnome = import('gnome')

gir = find_program('g-ir-scanner', required: get_option('introspection'))
build_gir = gir.found() and get_option('introspection').enabled()

if build_gir
  gir_args = ['-DREPLIT_COMPILATION', '--quiet']

  libreplit_gir = gnome.generate_gir(libreplit,
    sources: [replit_sources, replit_headers],
    namespace: 'Replit',
    nsversion: api_version,
    identifier_prefix: 'Replit',
    symbol_prefix: 'replit',
    export_packages: 'libreplit',
    includes: ['Gio-2.0', 'Json-1.0', 'Soup-3.0'],
    header: 'libreplit/replit.h',
    install: true,
    dependencies: replit_deps,
    extra_args: gir_args,
    fatal_warnings: get_option('werror'),
  )
endif

pkg = import('pkgconfig')

pkg.generate(
  description: 'A C library for interacting with Replit and its API',
  libraries: libreplit,
  name: 'libreplit',
  filebase: 'libreplit-' + api_version,
  version: meson.project_version(),
  subdirs: 'libreplit',
  requires: [
    'glib-2.0',
    'json-glib-1.0',
    'libsoup-3.0',
  ],
  install_dir: join_paths(get_option('libdir'), 'pkgconfig'),
)
