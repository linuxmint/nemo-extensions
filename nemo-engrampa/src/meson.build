nemo_engrampa_sources = [
    'engrampa-module.c',
    'nemo-engrampa.c',
]

libnemo_engrampa = shared_library('nemo-engrampa',
    nemo_engrampa_sources,
    include_directories: rootInclude,
    dependencies: [
        libnemo,
    ],

    install: true,
    install_dir: libnemo_extension_dir,
)
