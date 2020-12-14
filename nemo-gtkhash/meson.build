project('nemo-gtkhash',
    'c',
    version: '4.8.0',
    meson_version: '>=0.50.0'
)

project_url = 'https://github.com/linuxmint/nemo-extensions'


gnome       = import('gnome')
pkgconfig   = import('pkgconfig')
cc = meson.get_compiler('c')

config = configuration_data()

################################################################################
# Find libnemo-extensions

libnemo = dependency('libnemo-extension', version: '>=2.0.0')
libnemo_extension_dir = libnemo.get_pkgconfig_variable('extensiondir')
libnemo_extension_ver = libnemo.version().split('.')

config.set('NEMO_VERSION_MAJOR', libnemo_extension_ver[0])
config.set('NEMO_VERSION_MINOR', libnemo_extension_ver[1])
config.set('NEMO_VERSION_MICRO', libnemo_extension_ver[2])

glib = dependency('glib-2.0', version: '>=2.14.0')

################################################################################
# Extension dependencies

gzip = find_program('gzip')

gtk3 = dependency('gtk+-3.0', version: '>=3.0')


if [ 'debug', 'relwithdebinfo'].contains(get_option('buildtype'))
    config.set('G_DISABLE_DEPRECATED', 1)
    config.set('GDK_DISABLE_DEPRECATED', 1)
    config.set('GTK_DISABLE_DEPRECATED', 1)
else
    config.set('G_DISABLE_ASSERT', 1)
    config.set('G_DISABLE_CAST_CHECKS', 1)
endif

no_dep = dependency('', required: false)

zlib = get_option('zlib') ? dependency('zlib') : no_dep
config.set10('ENABLE_ZLIB', zlib.found())

libgcrypt = get_option('gcrypt') ? dependency('libgcrypt') : no_dep
config.set10('ENABLE_GCRYPT', libgcrypt.found())

libcrypto = get_option('libcrypto') ? dependency('libcrypto') : no_dep
config.set10('ENABLE_LIBCRYPTO', libcrypto.found())

nettle = get_option('nettle') ? dependency('nettle') : no_dep
config.set10('ENABLE_NETTLE', nettle.found())

nss = get_option('nss') ? dependency('nss') : no_dep
config.set10('ENABLE_NSS', nss.found())

mhash = get_option('mhash') ? cc.find_library('mhash', has_headers: 'mhash.h') : no_dep
config.set10('ENABLE_MHASH', mhash.found())


if get_option('linux-crypto')
    if not cc.has_header('linux/if_alg.h')
        error('Header if_alg.h not found !')
    endif
endif
config.set10('ENABLE_LINUX_CRYPTO', get_option('linux-crypto'))


if get_option('mbedtls')
    mbedtls = cc.find_library('mbedcrypto', has_headers: 'mbedtls/md.h', required: false)
    mbedtls_version = 2
    if not mbedtls.found()
        mbedtls = cc.find_library('polarssl', has_headers: 'polarssl/md.h', required: false)
        mbedtls_version = 1
    endif
    if not mbedtls.found()
        error('Could not find mbedtls or polarssl !')
    endif
else
    mbedtls = no_dep
endif
config.set10('ENABLE_MBEDTLS', mbedtls.found())
config.set10('HAVE_MBEDTLS_2_0_0', mbedtls_version == 2)


config.set10('ENABLE_GLIB_CHECKSUMS', get_option('glib-checksums'))

config.set10('ENABLE_MD6', get_option('internal-md6'))


build_gtkhash   = get_option('build-gtkhash')
build_nautilus  = get_option('build-nautilus')
build_nemo      = get_option('build-nemo')
build_thunar    = get_option('build-thunar')


config.set('HASH_FILE_BUFFER_SIZE', 131072)
config.set('HASH_FILE_REPORT_INTERVAL', 166)

################################################################################
# Generic stuff

config.set_quoted('GETTEXT_PACKAGE', 'nemo-extensions')
config.set_quoted('GNOMELOCALEDIR', get_option('prefix')/get_option('datadir')/'locale')

add_project_arguments('-DG_LOG_DOMAIN="nemo-gtkhash"', language: 'c')

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')
configure_file(
    output: 'config.h',
    configuration: config,
)

rootInclude = include_directories('.')

subdir('src')
subdir('data')



# -fno-common \
# -fvisibility=hidden \
# -pedantic \
# -Wall \
# -Wextra \
# -Waggregate-return \
# -Wbad-function-cast \
# -Wcast-align \
# -Winit-self \
# -Wfloat-equal \
# -Wlogical-op \
# -Wmissing-declarations \
# -Wmissing-noreturn \
# -Wredundant-decls \
# -Wshadow \
# -Wswitch-default \
# -Wwrite-strings \
# -Werror=implicit-function-declaration
