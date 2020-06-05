dbus_conf = configuration_data()

dbus_conf.set(
  'bindir',
  bindir,
)

configure_file(
  input: 'org.nemo.Preview.service.in',
  output: 'org.nemo.Preview.service',
  configuration: dbus_conf,
  install_dir: join_paths(datadir, 'dbus-1', 'services'),
)
