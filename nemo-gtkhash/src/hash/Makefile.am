noinst_LTLIBRARIES = hash.la

hash_la_SOURCES = \
	digest.c digest.h \
	digest-format.h \
	hash-file.c hash-file.h \
	hash-func.c hash-func.h \
	hash-lib.c hash-lib.h \
	hash-string.c hash-string.h \
	hmac.c hmac.h

hash_la_CPPFLAGS = $(GLIB_CFLAGS) -DIN_HASH_LIB

hash_la_CFLAGS = $(GTKHASH_CFLAGS)

hash_la_LDFLAGS = -avoid-version -module

hash_la_LIBADD = $(GLIB_LIBS)

EXTRA_DIST = \
	md6/md6_compress.c md6/md6_mode.c \
	mutils/mhash_config.h

if ENABLE_GCRYPT
hash_la_SOURCES += hash-lib-gcrypt.c hash-lib-gcrypt.h
hash_la_CPPFLAGS += $(LIBGCRYPT_CFLAGS)
hash_la_LIBADD += $(LIBGCRYPT_LIBS)
endif

if ENABLE_GLIB_CHECKSUMS
hash_la_SOURCES += hash-lib-glib.c hash-lib-glib.h
endif

if ENABLE_LIBCRYPTO
hash_la_SOURCES += hash-lib-crypto.c hash-lib-crypto.h
hash_la_CPPFLAGS += $(LIBCRYPTO_CFLAGS)
hash_la_LIBADD += $(LIBCRYPTO_LIBS)
endif

if ENABLE_LINUX_CRYPTO
hash_la_SOURCES += hash-lib-linux.c hash-lib-linux.h
endif

if ENABLE_MD6
hash_la_SOURCES += hash-lib-md6.c hash-lib-md6.h md6/md6.c md6/md6.h
endif

if ENABLE_MHASH
hash_la_SOURCES += hash-lib-mhash.c hash-lib-mhash.h
hash_la_LIBADD += $(MHASH_LIBS)
endif

if ENABLE_NETTLE
hash_la_SOURCES += hash-lib-nettle.c hash-lib-nettle.h
hash_la_CPPFLAGS += $(NETTLE_CFLAGS)
hash_la_LIBADD += $(NETTLE_LIBS)
endif

if ENABLE_NSS
hash_la_SOURCES += hash-lib-nss.c hash-lib-nss.h
hash_la_CPPFLAGS += $(NSS_CFLAGS)
hash_la_LIBADD += $(NSS_LIBS)
endif

if ENABLE_MBEDTLS
hash_la_SOURCES += hash-lib-mbedtls.c hash-lib-mbedtls.h
hash_la_LIBADD += $(MBEDTLS_LIBS)
endif

if ENABLE_ZLIB
hash_la_SOURCES += hash-lib-zlib.c hash-lib-zlib.h
hash_la_CPPFLAGS += $(ZLIB_CFLAGS)
hash_la_LIBADD += $(ZLIB_LIBS)
endif
