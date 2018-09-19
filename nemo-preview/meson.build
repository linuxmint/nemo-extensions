project('nemo-preview',
  'c',
  version: '3.8.0',
  meson_version: '>=0.40.0'
)
project_url = 'https://github.com/linuxmint/nemo-extensions'


gnome     = import('gnome')
pkgconfig = import('pkgconfig')

conf = configuration_data()

################################################################################
# Nemo-extensions generic

gmodule   = dependency('gmodule-2.0',       version: '>=2.14.0')
libnemoext= dependency('libnemo-extension', version: '>=1.0.0')

nemo_extensiondir = libnemoext.get_pkgconfig_variable('extensiondir')
nemo_libdir       = libnemoext.get_pkgconfig_variable('libdir')
nemo_extensionver = libnemoext.version().split('.')
conf.set_quoted('NEMO_EXTENSION_DIR', nemo_extensiondir)
conf.set_quoted('NEMO_LIBDIR',        nemo_libdir)

conf.set('NEMO_VERSION_MAJOR', nemo_extensionver[0])
conf.set('NEMO_VERSION_MINOR', nemo_extensionver[1])
conf.set('NEMO_VERSION_MICRO', nemo_extensionver[2])

################################################################################
# Check for Python


freetype2       = dependency('freetype2')
glib2           = dependency('glib-2.0', version: '>=2.37.3')
gobjectintrosp  = dependency('gobject-introspection-1.0', version: '>=0.9.2')
cjs             = dependency('cjs-1.0', version: '>=1.9.0')
gobject         = dependency('gobject-2.0', version: '>=2.37.3')
gtk3            = dependency('gtk+-3.0', version: '>=3.5.12')
clutter         = dependency('clutter-1.0', version: '>=1.11.4')
clutter_x11     = dependency('clutter-x11-1.0', version: '>=1.11.4')
clutter_gtk     = dependency('clutter-gtk-1.0', version: '>=1.0.1')
clutter_gst     = dependency('clutter-gst-3.0')
gstreamer       = dependency('gstreamer-1.0')
gstreamer_pb    = dependency('gstreamer-pbutils-1.0')
gstreamer_tag   = dependency('gstreamer-tag-1.0')
libmusicbrainz5 = dependency('libmusicbrainz5')
xreader_doc     = dependency('xreader-document-1.5')
xreader_view    = dependency('xreader-view-1.5')
gtksourceview   = dependency('gtksourceview-3.0')
webkit2gtk      = dependency('webkit2gtk-4.0')


################################################################################
# Generic stuff

gettext_package = meson.project_name()
conf.set_quoted('GETTEXT_PACKAGE', gettext_package)
conf.set_quoted('GNOMELOCALEDIR',  join_paths(
  get_option('prefix'), get_option('datadir'), 'locale'))

conf.set_quoted('NEMO_PREVIEW_PKGDATADIR', join_paths(get_option('datadir'), meson.project_name()))

configure_file(
  output: 'config.h',
  configuration: conf,
)

rootInclude = include_directories('.')

subdir('src')
subdir('po')
subdir('data')
