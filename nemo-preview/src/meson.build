gnome = import('gnome')

nemo_preview_sources = [
  'main.c',
]

deps = [
  clutter_dep,
  clutter_gst_dep,
  clutter_gtk_dep,
  xreader_document_dep,
  xreader_view_dep,
  freetype_dep,
  gdk_pixbuf_dep,
  cjs_dep,
  glib_dep,
  gstreamer_dep,
  gstreamer_pbutils_dep,
  gstreamer_tag_dep,
  gtk_dep,
  gtksourceview_dep,
  introspection_dep,
  musicbrainz_dep,
  webkit_dep,
]

# This actually doesn't do anything,
# but main.c includes config.h, so we need to have some value in it
conf_h = configuration_data()
conf_h.set_quoted(
  'PACKAGE_NAME',
  'nemo-preview',
)

configure_file(
  output: 'config.h',
  configuration: conf_h,
)

# Include the just generated config.h
include_dirs = [
  include_directories('.'),
]

# Compile libnemo-preview and its introspection data
subdir('libnemo-preview')

nemo_preview_resources = gnome.compile_resources(
    'nemo-preview-resources',
    'org.nemo.Preview.gresource.xml',
    c_name: 'nemo_preview',
)

nemo_preview_sources += nemo_preview_resources

executable(
  'nemo-preview-start',
  nemo_preview_sources,
  # link against the just built libnemo-preview
  dependencies: libnemo_preview_shared_link,
  install: true,
  install_dir: libexecdir,
)

conf = configuration_data()

conf.set(
  'pkgdatadir',
  join_paths(datadir, 'nemo-preview'),
)
conf.set(
  'pkglibdir',
  join_paths(libdir, 'nemo-preview'),
)
conf.set(
  'libexecdir',
  libexecdir,
)

nemo_preview_bin = configure_file(
  input: 'nemo-preview.in',
  output: 'nemo-preview',
  configuration: conf,
)

install_data(
  nemo_preview_bin,
  install_mode: 'rwxr-xr-x',
  install_dir: bindir,
)

# Install the Javascript components without installing path.js.in as well
install_subdir(
  'js',
  install_dir: join_paths(datadir, 'nemo-preview'),
  exclude_files: 'util/path.js.in',
)

js_conf = configuration_data()

js_conf.set(
  'localedir',
  join_paths(datadir, 'locale'),
)

js_conf.set(
  'pkgdatadir',
  join_paths(datadir, 'nemo-preview'),
)

configure_file(
  input: 'js/util/path.js.in',
  output: 'path.js',
  configuration: js_conf,
  install_dir: join_paths(datadir, 'nemo-preview', 'js', 'util'),
)
