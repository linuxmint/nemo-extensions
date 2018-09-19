
libnemo_share = shared_library('nemo-share',
  [
    'shares.c',
    'nemo-share.c',
  ],
  include_directories: rootInclude,
  dependencies: [
    glib,
    libnemoext,
    libcinnamon,
  ],

  install: true,
  install_dir: nemo_extensiondir,
)


# Install data
install_data('install-samba',
  install_dir: join_paths(get_option('datadir'), meson.project_name())
)

data_conf = configuration_data()
data_conf.set('pkgdatadir', join_paths(
  get_option('prefix'), get_option('datadir'), meson.project_name()
))

install_data(
  configure_file(
    input : 'org.nemo.share.samba_install.policy.in',
    output: 'org.nemo.share.samba_install.policy',
    configuration: data_conf,
  ),
  install_dir: join_paths(get_option('datadir'), 'polkit-1', 'actions')
)
