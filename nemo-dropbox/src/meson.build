
libnemo_dropbox = shared_library('nemo-dropbox',
  [
    'dropbox-client-util.c',
    'dropbox-client.c',
    'dropbox-command-client.c',
    'dropbox.c',
    'nemo-dropbox-hooks.c',
    'nemo-dropbox.c',
  ],
  dependencies: [
    glib,
    libnemo,
  ],

  c_args: [
    '-DEMBLEMDIR="@0@"'.format(join_paths(get_option('prefix'), emblems_dir)),
  ],

  install: true,
  install_dir: libnemo_extension_dir,
)
