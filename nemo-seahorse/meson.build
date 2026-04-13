project('nemo-seahorse',
  version: '6.6.0',
  meson_version: '>=0.56.0'
)

project_url = 'https://github.com/linuxmint/nemo-extensions'

install_data(
    'tool/nemo-seahorse-tool',
    install_dir: get_option('bindir'),
    install_mode: 'rwxr-xr-x',
)

subdir('data')
