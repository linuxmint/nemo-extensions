libnemo_preview_c = [
  'nemo-preview-cover-art.c',
  'nemo-preview-file-loader.c',
  'nemo-preview-font-loader.c',
  'nemo-preview-font-widget.c',
  'nemo-preview-pdf-loader.c',
  'nemo-preview-sound-player.c',
  'nemo-preview-text-loader.c',
  'nemo-preview-utils.c',
]

libnemo_preview_headers = [
  'nemo-preview-cover-art.h',
  'nemo-preview-file-loader.h',
  'nemo-preview-font-loader.h',
  'nemo-preview-font-widget.h',
  'nemo-preview-pdf-loader.h',
  'nemo-preview-sound-player.h',
  'nemo-preview-text-loader.h',
  'nemo-preview-utils.h',
]

libnemo_preview_enum = gnome.mkenums(
  'libnemo_preview_enum',
  c_template: 'nemo-preview-enum-types.c.in',
  h_template: 'nemo-preview-enum-types.h.in',
  sources: libnemo_preview_headers,
)

libnemo_preview = shared_library(
  'nemo-preview-1.0',
  dependencies: deps,
  sources: [
    libnemo_preview_c,
    libnemo_preview_enum,
    libnemo_preview_headers,
  ],
  install: true,
  install_dir: join_paths(libdir, 'nemo-preview'),
  include_directories: include_dirs,
)

# We use that later on to link the nemo-preview-start executable against libnemo_preview
libnemo_preview_shared_link = declare_dependency(
  link_with: libnemo_preview,
  dependencies: deps,
)

gnome.generate_gir(
  libnemo_preview,
  sources: [
    libnemo_preview_c,
    libnemo_preview_enum,
    libnemo_preview_headers,
  ],
  nsversion: '1.0',
  namespace: 'NemoPreview',
  symbol_prefix: 'nemo_preview',
  identifier_prefix: 'NemoPreview',
  includes: [
    'Clutter-1.0',
    'GstPbutils-1.0',
    'GstTag-1.0',
    'GdkPixbuf-2.0',
    'Gtk-3.0',
    'GtkSource-4',
    'XreaderDocument-1.5',
  ],
  install: true,
  install_dir_gir: join_paths(datadir, 'nemo-preview', 'gir-1.0'),
  install_dir_typelib: join_paths(libdir, 'nemo-preview', 'girepository-1.0'),
)
