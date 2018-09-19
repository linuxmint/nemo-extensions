project('nemo-repairer',
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
libnemoext= dependency('libnemo-extension', version: '>=1.0.0')

nemo_extensiondir = libnemoext.get_pkgconfig_variable('extensiondir')
nemo_libdir       = libnemoext.get_pkgconfig_variable('libdir')
nemo_extensionver = libnemoext.version().split('.')
conf.set_quoted('NEMO_EXTENSION_DIR', nemo_extensiondir)
conf.set_quoted('NEMO_LIBDIR',        nemo_libdir)

conf.set('NEMO_VERSION_MAJOR', nemo_extensionver[0])
conf.set('NEMO_VERSION_MINOR', nemo_extensionver[1])
conf.set('NEMO_VERSION_MICRO', nemo_extensionver[2])

################################################################################
# Dependencies

glib = dependency('glib-2.0')
gtk3 = dependency('gtk+-3.0')

################################################################################
# Generic stuff

gettext_package = meson.project_name()
conf.set_quoted('GETTEXT_PACKAGE', gettext_package)
conf.set_quoted('GNOMELOCALEDIR',  join_paths(
  get_option('prefix'), get_option('datadir'), 'locale'))

conf.set_quoted('PKGDATADIR', join_paths(
  get_option('prefix'), get_option('datadir'), meson.project_name()))

configure_file(
  output: 'config.h',
  configuration: conf,
)

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')
rootInclude = include_directories('.')

subdir('src')
subdir('po')
