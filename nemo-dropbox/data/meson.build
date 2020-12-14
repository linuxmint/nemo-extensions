install_subdir('emblems',
    install_dir: project_datadir,
)

if get_option('install-dropbox-files')
    install_subdir('icons/hicolor',
        install_dir: get_option('datadir') / 'icons',
    )
    install_data('icons/nemo-dropbox/nemo-dropbox-symbolic.svg',
        install_dir: project_datadir / 'icons',
    )
endif
