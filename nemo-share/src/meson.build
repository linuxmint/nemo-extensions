
libnemo_share = shared_library('nemo-share',
    'shares.c',
    'nemo-share.c',
    include_directories: rootInclude,
    dependencies: [
        glib,
        libnemo,
        libcinnamon,
    ],

    install: true,
    install_dir: libnemo_extension_dir,
)


# Install data
install_data(
    'install-samba',
    'check-directory-permissions',
    install_dir: get_option('datadir') / meson.project_name(),
    install_mode: 'rwxr-xr-x'
)

data_conf = configuration_data()
data_conf.set('pkgdatadir',
    get_option('prefix') / get_option('datadir') / meson.project_name()
)

install_data(
    configure_file(
        input : 'org.nemo.share.samba_install.policy.in',
        output: 'org.nemo.share.samba_install.policy',
        configuration: data_conf,
    ),
    install_dir: get_option('datadir') / 'polkit-1' / 'actions'
)
