project(
  'nemo-preview',
  'c',
  version: '4.8.0',
  meson_version: '>=0.42.0'
)

clutter_dep = dependency('clutter-1.0', version: '>=1.11.4')
clutter_gst_dep = dependency('clutter-gst-3.0')
clutter_gtk_dep = dependency('clutter-gtk-1.0', version: '>=1.0.1')
xreader_document_dep = dependency('xreader-document-1.5')
xreader_view_dep = dependency('xreader-view-1.5')
freetype_dep = dependency('freetype2')
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>=2.23.0')
cjs_dep = dependency('cjs-1.0', version: '>=1.9.0')
glib_dep = dependency('glib-2.0', version: '>=2.37.3')
gstreamer_dep = dependency('gstreamer-1.0')
gstreamer_base_dep = dependency('gstreamer-base-1.0')
gstreamer_pbutils_dep = dependency('gstreamer-pbutils-1.0')
gstreamer_tag_dep = dependency('gstreamer-tag-1.0')
gtk_dep = dependency('gtk+-3.0', version: '>=3.5.12')
gtksourceview_dep = dependency('gtksourceview-4', version: '>=4.0.3')
introspection_dep = dependency('gobject-introspection-1.0', version: '>=0.9.2')
musicbrainz_dep = dependency('libmusicbrainz5')
webkit_dep = dependency('webkit2gtk-4.0')

bindir = join_paths(get_option('prefix'), get_option('bindir'))
datadir = join_paths(get_option('prefix'), get_option('datadir'))
libdir = join_paths(get_option('prefix'), get_option('libdir'))
libexecdir = join_paths(get_option('prefix'), get_option('libexecdir'))

add_project_arguments(
  '-DNEMO_PREVIEW_PKGDATADIR="' + join_paths(datadir, 'nemo-preview') + '"',
  '-DGETTEXT_PACKAGE="' + meson.project_name() + '"',
  language: 'c',
)

subdir('data')
subdir('po')
subdir('src')
