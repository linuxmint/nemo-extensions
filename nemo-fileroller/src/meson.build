nemo_fileroller_sources = [
    'fileroller-module.c',
    'nemo-fileroller.c',
]

libnemo_fileroller = shared_library('nemo-fileroller',
    nemo_fileroller_sources,
    include_directories: rootInclude,
    dependencies: [
        libnemo,
    ],

    install: true,
    install_dir: libnemo_extension_dir,
)
