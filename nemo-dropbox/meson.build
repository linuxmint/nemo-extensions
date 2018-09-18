# https://github.com/linuxmint/nemo-extensions

project(
  'nemo-dropbox',
  'c',
  version: '3.6.0',
  meson_version: '>=0.43.0'
)

################################################################################
# Nemo-extensions generic

pkgconfig = import('pkgconfig')
python3mod= import('python3')

glib    = dependency('glib-2.0',          version: '>=2.14.0')

libnemo = dependency('libnemo-extension', version: '>=1.0.0')
libnemo_extension_dir = libnemo.get_pkgconfig_variable('extensiondir')
libnemo_extension_ver = libnemo.version().split('.')

global_args = [
  '-DPACKAGE_STRING="@0@"'  .format(meson.project_name()),
  '-DNEMO_VERSION_MAJOR=@0@'.format(libnemo_extension_ver[0]),
  '-DNEMO_VERSION_MINOR=@0@'.format(libnemo_extension_ver[1]),
  '-DNEMO_VERSION_MICRO=@0@'.format(libnemo_extension_ver[2]),
]

global_args += meson.get_compiler('c').get_supported_arguments(
  '-Wno-deprecated-declarations'
)

# if DEBUG
global_args += true ? [
  '-DND_DEBUG',
] : [
  '-DG_DISABLE_ASSERT',
  '-DG_DISABLE_CHECKS',
]

add_project_arguments(global_args,
  language: [ 'c', ],
)

################################################################################
# Extension specific

emblems_dir = join_paths(get_option('datadir'), meson.project_name(), 'emblems')

subdir('data')
subdir('src')


install_data('COPYING',
  install_dir: join_paths(get_option('datadir'), 'licenses', meson.project_name()),
)

custom_target('dropbox_files',
  input : [
    'dropbox.in',
    'dropbox.txt.in',
  ],
  output: [
    'dropbox',
    'dropbox.1',
  ],
  command: [
    find_program('dropbox_files.sh'),
    meson.source_root(),
    meson.build_root(),
    meson.project_version(),
    get_option('datadir'),
  ],
)
