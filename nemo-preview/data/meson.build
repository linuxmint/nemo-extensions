
install_subdir('style',
  install_dir: join_paths(get_option('datadir'), meson.project_name()),
  strip_directory: false,
)

dataconf = configuration_data()
dataconf.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))

configure_file(
  input : 'org.nemo.Preview.service.in',
  output: 'org.nemo.Preview.service',
  configuration: dataconf,
  install_dir: join_paths(get_option('datadir'), 'dbus-1', 'services')
)
