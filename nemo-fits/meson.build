project('nemo-fits',
  'c',
  version: '6.7.0',
  meson_version: '>=0.56.0'
)

project_url = 'https://github.com/linuxmint/nemo-extensions'

i18n = import('i18n')
gnome = import('gnome')

# Dependencies
glib_dep         = dependency('glib-2.0',         version: '>= 2.50.0')
gtk_dep          = dependency('gtk+-3.0',          version: '>= 3.22.0')
gdk_pixbuf_dep   = dependency('gdk-pixbuf-2.0')
nemo_ext_dep     = dependency('libnemo-extension', version: '>= 3.0.0')
cfitsio_dep      = dependency('cfitsio',           version: '>= 3.0')
m_dep            = meson.get_compiler('c').find_library('m', required: true)

# Configuration
conf_data = configuration_data()
conf_data.set_quoted('VERSION',         meson.project_version())
conf_data.set_quoted('GETTEXT_PACKAGE', 'nemo-extensions')
conf_data.set_quoted('LOCALEDIR',       join_paths(get_option('prefix'),
                                                   get_option('localedir')))

config_h = configure_file(
  output: 'config.h',
  configuration: conf_data
)

include_dirs = include_directories('.')

# ---------------------------------------------------------------------------
# Shared image-processing library (fits-autostretch + fits-render).
# Built as a static library so both targets get their own copy without
# requiring a separate installed shared library.
# ---------------------------------------------------------------------------

image_lib = static_library('fits-image',
  sources: [
    'fits-autostretch.c',
    'fits-render.c',
  ],
  dependencies: [glib_dep, gdk_pixbuf_dep, cfitsio_dep, m_dep],
  include_directories: include_dirs,
  pic: true,   # required because the extension is a shared module
)

image_lib_dep = declare_dependency(
  link_with: image_lib,
  include_directories: include_dirs,
)

# ---------------------------------------------------------------------------
# Nemo extension
# ---------------------------------------------------------------------------

shared_module('nemo-fits',
  sources: ['nemo-fits.c', config_h],
  dependencies: [glib_dep, gtk_dep, nemo_ext_dep, cfitsio_dep, m_dep,
                 image_lib_dep],
  include_directories: include_dirs,
  install: true,
  install_dir: join_paths(get_option('libdir'), 'nemo', 'extensions-3.0'),
  name_prefix: 'lib',
  name_suffix: 'so',
)

# ---------------------------------------------------------------------------
# Thumbnailer executable
# ---------------------------------------------------------------------------

executable('fits-thumbnailer',
  sources: ['fits-thumbnailer.c'],
  dependencies: [glib_dep, gdk_pixbuf_dep, cfitsio_dep, m_dep,
                 image_lib_dep],
  include_directories: include_dirs,
  install: true,
  install_dir: get_option('bindir'),
)

# ---------------------------------------------------------------------------
# Data files
# ---------------------------------------------------------------------------

install_data('fits.thumbnailer',
  install_dir: join_paths(get_option('datadir'), 'thumbnailers')
)