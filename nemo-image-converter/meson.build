project('nemo-image-convertert',
  'c',
  version: '3.8.0',
  meson_version: '>=0.40.0'
)
project_url = 'https://github.com/linuxmint/nemo-extensions'


gnome     = import('gnome')
pkgconfig = import('pkgconfig')

conf = configuration_data()

################################################################################
# Nemo-extensions generic

gmodule   = dependency('gmodule-2.0',       version: '>=2.14.0')
libnemoext= dependency('libnemo-extension', version: '>=2.0.8')

nemo_extensiondir = libnemoext.get_pkgconfig_variable('extensiondir')
nemo_libdir       = libnemoext.get_pkgconfig_variable('libdir')
nemo_extensionver = libnemoext.version().split('.')
conf.set_quoted('NEMO_EXTENSION_DIR', nemo_extensiondir)
conf.set_quoted('NEMO_LIBDIR',        nemo_libdir)

conf.set('NEMO_VERSION_MAJOR', nemo_extensionver[0])
conf.set('NEMO_VERSION_MINOR', nemo_extensionver[1])
conf.set('NEMO_VERSION_MICRO', nemo_extensionver[2])

################################################################################
# Check for Python

gtk3 = dependency('gtk+-3.0', version: '>=3.0')

################################################################################
# Generic stuff

conf.set_quoted('DATADIR', get_option('datadir'))
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('GETTEXT_PACKAGE', 'nemo-extensions')
conf.set_quoted('GNOMELOCALEDIR',  join_paths(
  get_option('prefix'), get_option('datadir'), 'locale'))


add_project_arguments('-DHAVE_CONFIG_H', language: 'c')
rootInclude = include_directories('.')
configure_file(
  output: 'config.h',
  configuration: conf,
)


subdir('src')
subdir('data')
subdir('po')
