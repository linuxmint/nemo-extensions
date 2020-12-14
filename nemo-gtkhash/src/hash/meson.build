hash_lib_sources = [
    'digest.c',
    'hash-file.c',
    'hash-func.c',
    'hash-lib.c',
    'hash-string.c',
    'hmac.c',
]

if get_option('gcrypt')
    hash_lib_sources += 'hash-lib-gcrypt.c'
endif
if get_option('glib-checksums')
    hash_lib_sources += 'hash-lib-glib.c'
endif
if get_option('libcrypto')
    hash_lib_sources += 'hash-lib-crypto.c'
endif
if get_option('linux-crypto')
    hash_lib_sources += 'hash-lib-linux.c'
endif
if get_option('internal-md6')
    hash_lib_sources += [ 'hash-lib-md6.c', 'md6/md6.c' ]
endif
if get_option('mhash')
    hash_lib_sources += 'hash-lib-mhash.c'
endif
if get_option('nettle')
    hash_lib_sources += 'hash-lib-nettle.c'
endif
if get_option('nss')
    hash_lib_sources += 'hash-lib-nss.c'
endif
if get_option('mbedtls')
    hash_lib_sources += 'hash-lib-mbedtls.c'
endif
if get_option('zlib')
    hash_lib_sources += 'hash-lib-zlib.c'
endif


hash_lib = static_library('hash',
    hash_lib_sources,
    include_directories: rootInclude,
    dependencies: [
        glib,
        libgcrypt,
        libcrypto,
        mhash,
        nettle,
        nss,
        mbedtls,
        zlib,
    ],
    c_args: [
        '-DIN_HASH_LIB',
    ],
)
