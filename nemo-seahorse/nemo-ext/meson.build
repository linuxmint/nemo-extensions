libnemo_seahorse_sources = [
    'nemo-seahorse-module.c',
    'nemo-seahorse.c',
]

libnemo_seahorse = library('nemo-seahorse',
    libnemo_seahorse_sources,
    include_directories: rootInclude,
    dependencies: [
        libnemo,
    ],
    install: true,
    install_dir: libnemo_extension_dir,
)
