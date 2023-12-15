project('nemo-image-converter',
    'c',
    version: '6.0.1',
    meson_version: '>=0.56.0'
)

project_url = 'https://github.com/linuxmint/nemo-extensions'


gnome       = import('gnome')
pkgconfig   = import('pkgconfig')

config = configuration_data()

################################################################################
# Find libnemo-extensions

libnemo = dependency('libnemo-extension', version: '>=2.0.0')
libnemo_extension_dir = libnemo.get_variable(pkgconfig: 'extensiondir')
libnemo_extension_ver = libnemo.version().split('.')

config.set('NEMO_VERSION_MAJOR', libnemo_extension_ver[0])
config.set('NEMO_VERSION_MINOR', libnemo_extension_ver[1])
config.set('NEMO_VERSION_MICRO', libnemo_extension_ver[2])

glib = dependency('glib-2.0', version: '>=2.14.0')

################################################################################
# Extension dependencies

gtk3 = dependency('gtk+-3.0', version: '>=3.0')

################################################################################
# Generic stuff

config.set_quoted('GETTEXT_PACKAGE', 'nemo-extensions')
config.set_quoted('PACKAGE', meson.project_name())
config.set_quoted('DATADIR',        get_option('prefix')/get_option('datadir'))
config.set_quoted('GNOMELOCALEDIR', get_option('prefix')/get_option('datadir')/'locale')

add_project_arguments('-DG_LOG_DOMAIN="Nemo-Share"', language: 'c')

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')
configure_file(
    output: 'config.h',
    configuration: config,
)

rootInclude = include_directories('.')

subdir('src')
subdir('data')
