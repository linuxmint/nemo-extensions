nemo_seahorse_sources = [
    'seahorse-notification.c',
    'seahorse-operation.c',
    'seahorse-passphrase.c',
    'seahorse-pgp-operation.c',
    'seahorse-progress.c',
    'seahorse-tool.c',
    'seahorse-tool-files.c',
    'seahorse-tool-progress.c',
    'seahorse-util.c',
    'seahorse-vfs-data.c',
    'seahorse-widget.c',
]

nemo_seahorse_sources += gnome.genmarshal('seahorse-marshal',
    prefix: 'seahorse_marshal',
    sources: 'seahorse-marshal.list',
)

nemo_seahorse = executable('nemo-seahorse-tool',
    nemo_seahorse_sources,
    include_directories: rootInclude,
    dependencies: [
        gtk3,
        gcr,
        cryptui,
        libgpgme,
        libnotify,
        avahi,
    ],
)

install_data(
    'seahorse-notify.xml',
    'seahorse-progress.xml',
    'seahorse-multi-encrypt.xml',
    install_dir: join_paths(get_option('datadir'), 'nemo-seahorse', 'ui'),
)
