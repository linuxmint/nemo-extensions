install_data([
    'constants.js',
    'totemMimeTypes.js',
  ],
  install_dir: join_paths(js_dir, 'util'),
)

configure_file(
  input : 'path.js.in',
  output: 'path.js',
  configuration: srcconf,
  install_dir: join_paths(js_dir, 'util'),
)
