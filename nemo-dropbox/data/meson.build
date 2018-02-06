install_subdir('emblems',
  install_dir: join_paths(emblems_dir, '..'),
)

if get_option('install-dropbox-files')
  install_subdir('icons',
    install_dir:  get_option('datadir'),
  )

  install_data('dropbox.desktop',
  install_dir: join_paths(get_option('datadir'), 'applications'),
  )
endif
