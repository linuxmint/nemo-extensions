dataconf = configuration_data()
dataconf.set('VERSION', meson.project_version())

install_data([
    'org.nemo.plugins.seahorse.gschema.xml',
    'org.nemo.plugins.seahorse.window.gschema.xml',
  ],
  install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas'),
)

install_man('nemo-seahorse-tool.1')


desktop_files = [
  'nemo-seahorse-pgp-encrypted.desktop',
  'nemo-seahorse-pgp-signature.desktop',
  'nemo-seahorse-pgp-keys.desktop',
]

foreach desktop_file : desktop_files
  desktop_file_in = configure_file(
    input : desktop_file + '.in.in',
    output: desktop_file + '.in',
    configuration: dataconf,
  )

  i18n.merge_file(
    input : desktop_file_in,
    output: desktop_file,
    po_dir: po_dir,
    type: 'desktop',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'applications'),
  )
endforeach
