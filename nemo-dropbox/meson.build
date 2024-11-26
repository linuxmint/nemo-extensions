project('nemo-dropbox',
    'c',
    version: '6.4.0',
    meson_version: '>=0.56.0'
)

project_url = 'https://github.com/linuxmint/nemo-extensions'


cc = meson.get_compiler('c')

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
# Project configuration

project_datadir = get_option('datadir') / meson.project_name()

config.set_quoted('GETTEXT_PACKAGE', 'nemo-extensions')
config.set_quoted('PACKAGE_STRING', meson.project_name() )
config.set_quoted('EMBLEMDIR',      get_option('prefix') / project_datadir / 'emblems')
config.set_quoted('NEMOICONDIR',    get_option('datadir') / 'nemo-dropbox' / 'icons')

c_args = []
c_args += cc.get_supported_arguments('-Wno-deprecated-declarations')

if [ 'debug', 'relwithdebinfo'].contains(get_option('buildtype'))
    c_args += [ '-DND_DEBUG', ]
else
    c_args += [ '-DG_DISABLE_ASSERT', '-DG_DISABLE_CHECKS', ]
endif

add_project_arguments(c_args, language: 'c')

################################################################################
# Generic stuff


add_project_arguments('-DHAVE_CONFIG_H', language: 'c')
configure_file(
    output: 'config.h',
    configuration: config,
)

rootInclude = include_directories('.')

subdir('data')
subdir('src')

install_data('COPYING',
    install_dir: get_option('datadir') / 'licenses' / meson.project_name(),
)
