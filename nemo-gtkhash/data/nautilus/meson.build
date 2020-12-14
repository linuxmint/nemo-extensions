
custom_target('gtkhash-properties.xml.gz',
    input:  'gtkhash-properties.xml',
    output: 'gtkhash-properties.xml.gz',
    command: [ gzip, '-c9', '@INPUT@' ],
    capture: true,
    install: true,
    install_dir: get_option('datadir') / meson.project_name() / 'nautilus'
)


install_data(
    'org.nemo.extensions.gtkhash.gschema.xml',
    install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
)
