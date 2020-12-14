if build_nautilus
    subdir('nautilus')
endif

if build_gtkhash

    custom_target('gtkhash.xml.gz',
        input:  'gtkhash.xml',
        output: 'gtkhash.xml.gz',
        command: [ gzip, '-c9', '@INPUT@' ],
        capture: true,
        install: true,
        install_dir: get_option('datadir') / meson.project_name()
    )

    install_data(
        'app.gtkhash.gschema.xml',
        install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
    )

endif
