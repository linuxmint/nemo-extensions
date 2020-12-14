nemo_dropbox_sources = [
    'dropbox-client-util.c',
    'dropbox-client.c',
    'dropbox-command-client.c',
    'dropbox.c',
    'nemo-dropbox-hooks.c',
    'nemo-dropbox.c',
]

libnemo_dropbox = shared_library('nemo-dropbox',
    nemo_dropbox_sources,
    include_directories: rootInclude,
    dependencies: [
        glib,
        libnemo,
    ],
    install: true,
    install_dir: libnemo_extension_dir,
)
