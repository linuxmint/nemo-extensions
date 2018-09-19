
libnemo_preview_sources = [
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

libnemo_preview_sources += gnome.mkenums('nemo-preview-enum-types',
  sources: libnemo_preview_headers,
  c_template: 'nemo-preview-enum-types.c.in',
  h_template: 'nemo-preview-enum-types.h.in',
)


libnemo_preview = library('nemo-preview-1.0',
  libnemo_preview_sources,

  include_directories: rootInclude,
  dependencies: [
    clutter,
    glib2,
    gtksourceview,
    freetype2,
    gstreamer_pb,
    gstreamer_tag,
    gstreamer,
    libmusicbrainz5,
    xreader_doc,
    xreader_view,
  ],

  c_args: [
    '-DPREFIX="@0@"'.format(get_option('prefix')),
    '-DLIBDIR="@0@"'.format(get_option('libdir')),
    '-DG_LOG_DOMAIN="Nemo-preview"',
    '-DG_DISABLE_DEPRECATED',
    '-DGST_USE_UNSTABLE_API',

  ],

  install_dir: join_paths(get_option('libdir'), meson.project_name()),
)

gnome.generate_gir(libnemo_preview,
  namespace: 'NemoPreview',
  nsversion: '1.0',
  symbol_prefix: 'nemo_preview',
  identifier_prefix: 'NemoPreview',
  includes: [
    'Clutter-1.0',
    'GdkPixbuf-2.0',
    'GstPbutils-1.0',
    'GstTag-1.0',
    'Gtk-3.0',
    'GtkSource-3.0',
    'XreaderDocument-1.5',
    'XreaderView-1.5',
  ],
  sources: [
    libnemo_preview_sources,
    libnemo_preview_headers,
  ],
  install: true,
)
