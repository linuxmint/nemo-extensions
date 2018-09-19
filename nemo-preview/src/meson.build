srcconf = configuration_data()
srcconf.set('pkgdatadir', join_paths(get_option('prefix'), ('datadir'),meson.project_name()))
srcconf.set('pkglibdir',  join_paths(get_option('prefix'), ('libdir'), meson.project_name()))
srcconf.set('libexecdir', join_paths(get_option('prefix'), ('libexecdir')))
srcconf.set('localedir',  join_paths(get_option('prefix'), ('localedir')))

subdir('libnemo-preview')
subdir('js')

nemo_preview = configure_file(
  input : 'nemo-preview.in',
  output: 'nemo-preview',
  configuration: srcconf,
)


nemo_preview_start = executable('nemo-preview-start',
  'nemo-preview-start.c',
  include_directories: rootInclude,
  dependencies: [
    glib2,
    gobjectintrosp,
    cjs,
    clutter,
    clutter_gst,
    clutter_gtk,
  ],
  link_with: libnemo_preview,
  install_dir: join_paths(get_option('libdir'), meson.project_name()),
)
