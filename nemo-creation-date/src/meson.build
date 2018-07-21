nemo_min_ver = '>=3.8.0'

libdeps = []

libnemo = dependency('libnemo-extension', version: nemo_min_ver, required: true)

libdeps += libnemo
libdeps += dependency('glib-2.0', required: true)

headers = [
    'nemo-creation-date.h',
    'statx.h'
]

sources = [
    'nemo-creation-date.c',
    'statx.c'
]

mod = shared_module(meson.project_name(),
    sources  : headers + sources,
    include_directories: [top_inc],
    dependencies: libdeps,
    install_dir: libnemo.get_pkgconfig_variable('extensiondir'),
    install: true
)

