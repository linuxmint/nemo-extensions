sources = [
    'nemo-python.c',
    'nemo-python.h',
    'nemo-python-object.c',
    'nemo-python-object.h'
]

mod = shared_module('nemo-python',
    sources  : sources,
    include_directories: [top_inc],
    dependencies: deps,
    install_dir: nemo.get_pkgconfig_variable('extensiondir'),
    install: true
)

