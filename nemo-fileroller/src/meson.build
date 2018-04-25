
libnemo_fileroller = shared_library('nemo-fileroller',
  [
    'fileroller-module.c',
    'nemo-fileroller.c',
  ],
  include_directories: rootInclude,
  dependencies: [
    libnemoext,
    libpython,
    pygobject,
    gmodule,
  ],

  install: true,
  install_dir: nemo_extensiondir,
)
