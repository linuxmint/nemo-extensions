dataconf = configuration_data()
dataconf.set('VERSION', meson.project_version())

install_data(
    'org.nemo.plugins.seahorse.gschema.xml',
    'org.nemo.plugins.seahorse.window.gschema.xml',
    install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
)

install_man('nemo-seahorse-tool.1')
