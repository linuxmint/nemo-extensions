project('nemo-share',
  'c',
  version: '3.8.0',
  meson_version: '>=0.40.0'
)
project_url = 'https://github.com/linuxmint/nemo-extensions'


gnome     = import('gnome')
pkgconfig = import('pkgconfig')
i18n      = import('i18n')

conf = configuration_data()

################################################################################
# Nemo-extensions generic

glib      = dependency('glib-2.0',          version: '>=2.35.9')
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
# Specific stuff to the extension

gettext_package = meson.project_name()
log_domain = 'Nemo-Share'

libcinnamon = dependency('cinnamon-desktop', version: '>=3.0.0')


interfaces_dir = join_paths(get_option('datadir'), meson.project_name(), 'interfaces')

conf.set_quoted('GETTEXT_PACKAGE', gettext_package)

conf.set_quoted('INTERFACES_DIR', join_paths(get_option('prefix'), interfaces_dir))
conf.set_quoted('NEMO_SHARE_LOCALEDIR', get_option('localedir'))
conf.set_quoted('PKGDATADIR', join_paths(
  get_option('prefix'), get_option('datadir'), meson.project_name())
)

configure_file(
  output: 'config.h',
  configuration: conf,
)

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')
rootInclude = include_directories('.')

subdir('interfaces')
subdir('po')
subdir('src')
