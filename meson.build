project('replit', 'c',
          version: '0.1.0',
    meson_version: '>= 0.59.0',
  default_options: [ 'warning_level=2',
                     'werror=false',
                     'c_std=gnu11',
                   ],
)

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
configure_file(
  output: 'replit-config.h',
  configuration: config_h,
)
add_project_arguments([
  '-I' + meson.project_build_root(),
], language: 'c')

replit_prefix = get_option('prefix')
replit_datadir = join_paths(replit_prefix, get_option('datadir'))

subdir('libreplit')
subdir('libreplit-cli')

subdir('docs/reference')
