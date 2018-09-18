project('nemo-fileroller',
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

libpython = dependency('python2')
conf.set_quoted('PYTHON_VERSION', libpython.version())
conf.set_quoted('PY_LIB_LOC',     libpython.get_pkgconfig_variable('libdir'))

pygobject = dependency('pygobject-3.0', version: '>=3.0.0')

pygobject_extension_ver = pygobject.version().split('.')
conf.set('PYGOBJECT_MAJOR_VERSION', pygobject_extension_ver[0])
conf.set('PYGOBJECT_MINOR_VERSION', pygobject_extension_ver[1])
conf.set('PYGOBJECT_MICRO_VERSION', pygobject_extension_ver[2])


################################################################################
# Generic stuff

conf.set_quoted('GETTEXT_PACKAGE', 'nemo-extensions')
conf.set_quoted('GNOMELOCALEDIR',  join_paths(
  get_option('prefix'), get_option('datadir'), 'locale'))


configure_file(
  output: 'config.h',
  configuration: conf,
)

rootInclude = include_directories('.')

subdir('src')
