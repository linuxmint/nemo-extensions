libnemo_filename_repairer_sources = [
  'filename-repairer.c',
  'nemo-filename-repairer.c',
]

nemo_filename_repairer_sources = [
  'encoding-dialog.c',
  'repair-dialog.c',
  'repairer.c',
]


libnemo_filename_repairer = library('nemo-filename-repairer',
  libnemo_filename_repairer_sources,
  include_directories: rootInclude,
  dependencies: [
    gtk3,
    libnemoext,
  ],
  install: true,
  install_dir: nemo_extensiondir,
)

nemo_filename_repairer = executable('nemo-filename-repairer',
  nemo_filename_repairer_sources,
  include_directories: rootInclude,
  dependencies: [
    gtk3,
  ],
  install: true,
)

install_data([
    'encoding-dialog.ui',
    'repair-dialog.ui',
  ],
  install_dir: join_paths(get_option('datadir'), meson.project_name())
)
