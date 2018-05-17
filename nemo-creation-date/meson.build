project('nemo-creation-date',
    'c',
    version : '1.0.0'
)

cc = meson.get_compiler('c')

cdata = configuration_data()
cdata.set_quoted('GETTEXT_PACKAGE', 'nemo-extensions')
cdata.set_quoted('LOCALE_DIR', join_paths(get_option('prefix'), get_option('localedir')))

statx_supported = cc.compiles(
'''
#define _GNU_SOURCE
#include <linux/stat.h>

struct statx stxbuf;
'''
)

if not statx_supported
    message('WARNING: No native statx support (Kernel < 4.11) - using compatibility wrappers.')
else
    message('Native statx support used.')
endif

cdata.set10('NATIVE_STATX', statx_supported)

c = configure_file(output : 'config.h',
    configuration : cdata
)

top_inc = include_directories('.')

subdir('src')
