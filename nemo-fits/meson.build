project('nemo-fits', 
  'c',
  version: '6.6.0',
  meson_version: '>=0.56.0'
)

project_url = 'https://github.com/linuxmint/nemo-extensions'

i18n = import('i18n')
gnome = import('gnome')

# Dependencies
glib_dep = dependency('glib-2.0', version: '>= 2.50.0')
gtk_dep = dependency('gtk+-3.0', version: '>= 3.22.0')
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0')
nemo_extension_dep = dependency('libnemo-extension', version: '>= 3.0.0')
cfitsio_dep = dependency('cfitsio', version: '>= 3.0')

# Configuration - Use repository-wide translation domain
conf_data = configuration_data()
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set_quoted('GETTEXT_PACKAGE', 'nemo-extensions')
conf_data.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))

config_h = configure_file(
  output: 'config.h',
  configuration: conf_data
)

# Extension source files
extension_sources = files(
  'nemo-fits.c',
)

extension_headers = files(
  'nemo-fits.h',
)

extension_deps = [
  glib_dep,
  gtk_dep,
  nemo_extension_dep,
  cfitsio_dep,
]

# Include directories
include_dirs = include_directories('.')

# Build the extension as a shared module
shared_module('nemo-fits',
  sources: [extension_sources, extension_headers, config_h],
  dependencies: extension_deps,
  include_directories: include_dirs,
  install: true,
  install_dir: join_paths(get_option('libdir'), 'nemo', 'extensions-3.0'),
  name_prefix: 'lib',
  name_suffix: 'so'
)

# Build the thumbnailer executable
thumbnailer_sources = files('fits-thumbnailer.c')

executable('fits-thumbnailer',
  sources: thumbnailer_sources,
  dependencies: [glib_dep, gdk_pixbuf_dep, cfitsio_dep],
  install: true,
  install_dir: get_option('bindir')
)

# Install thumbnailer definition
install_data('fits.thumbnailer',
  install_dir: join_paths(get_option('datadir'), 'thumbnailers')
)