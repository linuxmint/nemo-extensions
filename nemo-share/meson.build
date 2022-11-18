project('nemo-share',
    'c',
    version: '5.6.0',
    meson_version: '>=0.49.0'
)

project_url = 'https://github.com/linuxmint/nemo-extensions'


gnome     = import('gnome')
pkgconfig = import('pkgconfig')

config = configuration_data()

################################################################################
# Find libnemo-extensions

libnemo = dependency('libnemo-extension', version: '>=2.0.0')
libnemo_extension_dir = libnemo.get_pkgconfig_variable('extensiondir')
libnemo_extension_ver = libnemo.version().split('.')

config.set('NEMO_VERSION_MAJOR', libnemo_extension_ver[0])
config.set('NEMO_VERSION_MINOR', libnemo_extension_ver[1])
config.set('NEMO_VERSION_MICRO', libnemo_extension_ver[2])

glib = dependency('glib-2.0', version: '>=2.14.0')

################################################################################
# Extension dependencies

libcinnamon = dependency('cinnamon-desktop', version: '>=3.0.0')

interfaces_dir = get_option('datadir') / meson.project_name() / 'interfaces'
config.set_quoted('INTERFACES_DIR', get_option('prefix') / interfaces_dir)

################################################################################
# Specific stuff to the extension

gettext_package = meson.project_name()
config.set_quoted('GETTEXT_PACKAGE', gettext_package)
config.set_quoted('NEMO_SHARE_LOCALEDIR', get_option('prefix') / get_option('localedir'))
config.set_quoted('PKGDATADIR', get_option('prefix')/get_option('datadir')/meson.project_name())

add_project_arguments('-DG_LOG_DOMAIN="Nemo-Share"', language: 'c')

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')
configure_file(
    output: 'config.h',
    configuration: config,
)

rootInclude = include_directories('.')

subdir('interfaces')
subdir('po')
subdir('src')
