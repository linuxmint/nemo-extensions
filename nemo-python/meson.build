project('python-nemo',
    'c',
    version : '6.6.0',
    meson_version : '>=0.56.0'
)

gnome = import('gnome')

deps = []

# In python 3.8, applications which embed python (i.e. not importable modules)
# must use python3-embed, instead of python3. Check that first.
# https://bugs.python.org/issue36721
python3 = dependency('python3-embed', required: false)
if not python3.found()
    python3 = dependency('python3')
endif
nemo = dependency('libnemo-extension', required: true)

deps += python3
deps += nemo
deps += dependency('pygobject-3.0')
deps += dependency('gtk+-3.0')
deps += dependency('glib-2.0')

cdata = configuration_data()

version_array = python3.version().split('.')
ret = run_command(['python3-config', '--abiflags'], check: true)

py_so_filename = 'libpython@0@.@1@@2@.so.1.0'.format(
    version_array[0],
    version_array[1],
    ret.stdout().strip()
)

libpath = join_paths(get_option('prefix'), get_option('libdir'), py_so_filename)
datadir = join_paths(get_option('prefix'), get_option('datadir'))
pyextdir = join_paths(get_option('prefix'), get_option('datadir'), 'nemo-python/extensions')

cdata.set_quoted('NEMO_EXTENSION_DIR', nemo.get_variable(pkgconfig: 'extensiondir'))
cdata.set_quoted('PYTHON_LIBPATH', libpath)
cdata.set('PYGOBJECT_MAJOR_VERSION', 3)
cdata.set('PYGOBJECT_MINOR_VERSION', 0)
cdata.set('PYGOBJECT_MICRO_VERSION', 0)
cdata.set_quoted('DATA_DIR', datadir)
cdata.set_quoted('PYTHON_EXTENSION_DIR', pyextdir)

c = configure_file(output : 'config.h',
    configuration : cdata
)

pc_conf = configuration_data()
    pc_conf.set('prefix', get_option('prefix'))
    pc_conf.set('version', meson.project_version())
    configure_file(
    input : 'nemo-python.pc.in',
    output: 'nemo-python.pc',
    configuration: pc_conf,
    install: true,
    install_dir: join_paths(get_option('libdir'), 'pkgconfig'),
)

top_inc = include_directories('.')

subdir('src')

gtkdoc_enabled = get_option('gtk_doc')

if gtkdoc_enabled
  find_program('gtkdoc-scan', required: true)
  subdir('docs')
endif

message('|....................................................................')
message('|')
message('|  nemo-python version: @0@'.format(meson.project_version()))
message('|')
message('|  python version: @0@'.format(python3.version()))
message('|  targeting lib: @0@'.format(libpath))
message('|')
message('|  build docs: @0@'.format(gtkdoc_enabled))
message('|')
message('|  python extension dir: @0@'.format(nemo.get_variable(pkgconfig: 'extensiondir')))
message('|')
message('|....................................................................')
