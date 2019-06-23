AC_PREREQ([2.61])
AC_INIT([nemo-gtkhash], [4.2.0])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11 check-news dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC_C99
AM_PROG_CC_C_O
LT_INIT

GTKHASH_CFLAGS=""

for opt in \
	-fno-common \
	-fvisibility=hidden \
	-pedantic \
	-Wall \
	-Wextra \
	-Waggregate-return \
	-Wbad-function-cast \
	-Wcast-align \
	-Winit-self \
	-Wfloat-equal \
	-Wlogical-op \
	-Wmissing-declarations \
	-Wmissing-noreturn \
	-Wredundant-decls \
	-Wshadow \
	-Wswitch-default \
	-Wwrite-strings \
	-Werror=implicit-function-declaration
do
	AC_MSG_CHECKING([whether "${CC}" accepts option "${opt}"])
	ac_save_CFLAGS="${CFLAGS}"
	CFLAGS="${GTKHASH_CFLAGS} ${opt} ${CFLAGS}"
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([], [])],
		[AC_MSG_RESULT([yes]) ; GTKHASH_CFLAGS="${GTKHASH_CFLAGS} ${opt}"],
		[AC_MSG_RESULT([no])])
	CFLAGS="${ac_save_CFLAGS}"
done

AC_SUBST([GTKHASH_CFLAGS])

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.36])
GLIB_GSETTINGS

AC_MSG_CHECKING([which GTK+ version to use])
AC_ARG_WITH([gtk],
	[AS_HELP_STRING([--with-gtk=2.0|3.0], [GTK+ version to use [2.0]])],
	[with_gtk="${withval}"], [with_gtk="2.0"])
AC_MSG_RESULT(["${with_gtk}"])

PKG_CHECK_MODULES([GTK], ["gtk+-${with_gtk}" >= 2.24])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

AC_MSG_CHECKING([whether to enable debugging])
AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug], [enable debugging])],
	[enable_debug="${enableval}"], [enable_debug="no"])
AC_MSG_RESULT(["${enable_debug}"])

if test "${enable_debug}" = "yes" ; then
	AC_DEFINE([G_DISABLE_DEPRECATED], [1], [disable glib deprecated])
	AC_DEFINE([GDK_DISABLE_DEPRECATED], [1], [disable gdk deprecated])
	AC_DEFINE([GTK_DISABLE_DEPRECATED], [1], [disable gtk+ deprecated])
else
	AC_DEFINE([G_DISABLE_ASSERT], [1], [disable glib assert])
	AC_DEFINE([G_DISABLE_CAST_CHECKS], [1], [disable glib cast checks])
fi

hash_libs=""

AC_MSG_CHECKING([whether to use zlib])
AC_ARG_ENABLE([zlib],
	[AS_HELP_STRING([--disable-zlib], [do not use zlib])],
	[enable_zlib="${enableval}"], [enable_zlib="yes"])
AC_MSG_RESULT(["${enable_zlib}"])
AM_CONDITIONAL([ENABLE_ZLIB], [test "${enable_zlib}" = "yes"])
AC_DEFINE([ENABLE_ZLIB], [0], [Use zlib])

if test "${enable_zlib}" = "yes" ; then
	PKG_CHECK_MODULES([ZLIB], [zlib])
	AC_SUBST([ZLIB_CFLAGS])
	AC_SUBST([ZLIB_LIBS])
	hash_libs="${hash_libs} zlib"
	AC_DEFINE([ENABLE_ZLIB], [1])
fi

AC_MSG_CHECKING([whether to use the Linux-2.6.38+ hash algorithm interface])
AC_ARG_ENABLE([linux-crypto],
	[AS_HELP_STRING([--enable-linux-crypto], [use the Linux-2.6.38+ hash algorithm interface])],
	[enable_linux_crypto="${enableval}"], [enable_linux_crypto="no"])
AC_MSG_RESULT(["${enable_linux_crypto}"])
AM_CONDITIONAL([ENABLE_LINUX_CRYPTO], [test "${enable_linux_crypto}" = "yes"])
AC_DEFINE([ENABLE_LINUX_CRYPTO], [0], [Use Linux Crypto])

if test "${enable_linux_crypto}" = "yes" ; then
	AC_CHECK_HEADER([linux/if_alg.h], [:], [AC_MSG_ERROR([linux/if_alg.h not found])])
	hash_libs="${hash_libs} linux_crypto"
	AC_DEFINE([ENABLE_LINUX_CRYPTO], [1])
fi

AC_MSG_CHECKING([whether to use Libgcrypt])
AC_ARG_ENABLE([gcrypt],
	[AS_HELP_STRING([--enable-gcrypt], [use Libgcrypt])],
	[enable_gcrypt="${enableval}"], [enable_gcrypt="no"])
AC_MSG_RESULT(["${enable_gcrypt}"])
AM_CONDITIONAL([ENABLE_GCRYPT], [test "${enable_gcrypt}" = "yes"])
AC_DEFINE([ENABLE_GCRYPT], [0], [Use Libgcrypt])

if test "${enable_gcrypt}" = "yes" ; then
	AM_PATH_LIBGCRYPT([1.6.1], [:], [AC_MSG_ERROR([Libgcrypt not found])])
	hash_libs="${hash_libs} gcrypt"
	AC_DEFINE([ENABLE_GCRYPT], [1])
fi

AC_MSG_CHECKING([whether to use libcrypto])
AC_ARG_ENABLE([libcrypto],
	[AS_HELP_STRING([--enable-libcrypto], [use libcrypto])],
	[enable_libcrypto="${enableval}"], [enable_libcrypto="no"])
AC_MSG_RESULT(["${enable_libcrypto}"])
AM_CONDITIONAL([ENABLE_LIBCRYPTO], [test "${enable_libcrypto}" = "yes"])
AC_DEFINE([ENABLE_LIBCRYPTO], [0], [Use libcrypto])

if test "${enable_libcrypto}" = "yes" ; then
	PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto])
	AC_SUBST([LIBCRYPTO_CFLAGS])
	AC_SUBST([LIBCRYPTO_LIBS])
	hash_libs="${hash_libs} libcrypto"
	AC_DEFINE([ENABLE_LIBCRYPTO], [1])
fi

AC_MSG_CHECKING([whether to use mbed TLS])
AC_ARG_ENABLE([mbedtls],
	[AS_HELP_STRING([--enable-mbedtls], [use mbed TLS])],
	[enable_mbedtls="${enableval}"], [enable_mbedtls="no"])
AC_MSG_RESULT(["${enable_mbedtls}"])
AM_CONDITIONAL([ENABLE_MBEDTLS], [test "${enable_mbedtls}" = "yes"])
AC_DEFINE([ENABLE_MBEDTLS], [0], [Use mbed TLS])
AC_DEFINE([HAVE_MBEDTLS_2_0_0], [0], [Have mbed TLS 2.x+])

if test "${enable_mbedtls}" = "yes" ; then
	AC_CHECK_HEADER([mbedtls/md.h], [have_mbedtls_2_0_0="yes"], [have_mbedtls_2_0_0="no"])
	AC_CHECK_HEADER([polarssl/md.h], [have_mbedtls_1_3_0="yes"], [have_mbedtls_1_3_0="no"])
	if test "${have_mbedtls_2_0_0}" != "yes" && test "${have_mbedtls_1_3_0}" != "yes"; then
		AC_MSG_ERROR([mbedtls headers not found])
	fi
	if test "${have_mbedtls_2_0_0}" = "yes"; then
		AC_CHECK_LIB([mbedcrypto], [mbedtls_md_update], [:], [AC_MSG_ERROR([mbedtls library not found])])
		MBEDTLS_LIBS="-lmbedcrypto"
		hash_libs="${hash_libs} mbedtls"
		AC_DEFINE([HAVE_MBEDTLS_2_0_0], [1])
	else
		AC_CHECK_LIB([polarssl], [md_update], [:], [AC_MSG_ERROR([mbedtls library not found])])
		MBEDTLS_LIBS="-lpolarssl"
		hash_libs="${hash_libs} polarssl"
	fi
	AC_SUBST([MBEDTLS_LIBS])
	AC_DEFINE([ENABLE_MBEDTLS], [1])
fi

AC_MSG_CHECKING([whether to use Nettle])
AC_ARG_ENABLE([nettle],
	[AS_HELP_STRING([--enable-nettle], [use Nettle])],
	[enable_nettle="${enableval}"], [enable_nettle="no"])
AC_MSG_RESULT(["${enable_nettle}"])
AM_CONDITIONAL([ENABLE_NETTLE], [test "${enable_nettle}" = "yes"])
AC_DEFINE([ENABLE_NETTLE], [0], [Use Nettle])

if test "${enable_nettle}" = "yes" ; then
	PKG_CHECK_MODULES([NETTLE], [nettle])
	AC_SUBST([NETTLE_CFLAGS])
	AC_SUBST([NETTLE_LIBS])
	hash_libs="${hash_libs} nettle"
	AC_DEFINE([ENABLE_NETTLE], [1])
fi

AC_MSG_CHECKING([whether to use NSS])
AC_ARG_ENABLE([nss],
	[AS_HELP_STRING([--enable-nss], [use NSS])],
	[enable_nss="${enableval}"], [enable_nss="no"])
AC_MSG_RESULT(["${enable_nss}"])
AM_CONDITIONAL([ENABLE_NSS], [test "${enable_nss}" = "yes"])
AC_DEFINE([ENABLE_NSS], [0], [Use NSS])

if test "${enable_nss}" = "yes" ; then
	PKG_CHECK_MODULES([NSS], [nss])
	AC_SUBST([NSS_CFLAGS])
	AC_SUBST([NSS_LIBS])
	hash_libs="${hash_libs} nss"
	AC_DEFINE([ENABLE_NSS], [1])
fi

AC_MSG_CHECKING([whether to use the Glib Data Checksums API])
AC_ARG_ENABLE([glib-checksums],
	[AS_HELP_STRING([--disable-glib-checksums], [do not use the Glib Data Checksums API])],
	[enable_glib_checksums="${enableval}"], [enable_glib_checksums="yes"])
AC_MSG_RESULT(["${enable_glib_checksums}"])
AM_CONDITIONAL([ENABLE_GLIB_CHECKSUMS], [test "${enable_glib_checksums}" = "yes"])
AC_DEFINE([ENABLE_GLIB_CHECKSUMS], [0], [Use Glib Checksums])

if test "${enable_glib_checksums}" = "yes" ; then
	hash_libs="${hash_libs} glib_checksums"
	AC_DEFINE([ENABLE_GLIB_CHECKSUMS], [1])
fi

AC_MSG_CHECKING([whether to use the mhash library])
AC_ARG_ENABLE([mhash],
	[AS_HELP_STRING([--enable-mhash], [use the mhash library])],
	[enable_mhash="${enableval}"], [enable_mhash="no"])
AC_MSG_RESULT(["${enable_mhash}"])
AM_CONDITIONAL([ENABLE_MHASH], [test "${enable_mhash}" = "yes"])
AC_DEFINE([ENABLE_MHASH], [0], [Use mhash])

if test "${enable_mhash}" = "yes" ; then
	AC_CHECK_HEADER([mhash.h], [:], [AC_MSG_ERROR([mhash.h not found])])
	AC_CHECK_LIB([mhash], [mhash], [:], [AC_MSG_ERROR([mhash library not found])])
	MHASH_LIBS="-lmhash"
	AC_SUBST([MHASH_LIBS])
	hash_libs="${hash_libs} mhash"
	AC_DEFINE([ENABLE_MHASH], [1])
fi

AC_MSG_CHECKING([whether to use the internal MD6 library])
AC_ARG_ENABLE([internal-md6],
	[AS_HELP_STRING([--disable-internal-md6], [do not use the internal MD6 library])],
	[enable_internal_md6="${enableval}"], [enable_internal_md6="yes"])
AC_MSG_RESULT(["${enable_internal_md6}"])
AM_CONDITIONAL([ENABLE_MD6], [test "${enable_internal_md6}" = "yes"])
AC_DEFINE([ENABLE_MD6], [0], [Use MD6])

if test "${enable_internal_md6}" = "yes" ; then
	hash_libs="${hash_libs} internal_md6"
	AC_DEFINE([ENABLE_MD6], [1])
fi

test -z "${hash_libs}" && AC_MSG_ERROR([No hash libraries enabled])

AC_MSG_CHECKING([whether to build gtkhash])
AC_ARG_ENABLE([gtkhash],
	[AS_HELP_STRING([--disable-gtkhash], [do not build gtkhash])],
	[enable_gtkhash="${enableval}"], [enable_gtkhash="yes"])
AC_MSG_RESULT(["${enable_gtkhash}"])
AM_CONDITIONAL([ENABLE_GTKHASH], [test "${enable_gtkhash}" = "yes"])

AC_MSG_CHECKING([whether to build the Nautilus extension])
AC_ARG_ENABLE([nautilus],
	[AS_HELP_STRING([--enable-nautilus], [build the Nautilus extension])],
	[enable_nautilus="${enableval}"], [enable_nautilus="no"])
AC_MSG_RESULT(["${enable_nautilus}"])
AM_CONDITIONAL([ENABLE_NAUTILUS], [test "${enable_nautilus}" = "yes"])

if test "${enable_nautilus}" = "yes" ; then
	# Check for nautilus
	PKG_CHECK_MODULES([NAUTILUS], [libnautilus-extension])
	AC_SUBST([NAUTILUS_CFLAGS])
	AC_SUBST([NAUTILUS_LIBS])
	NAUTILUS_EXTENSION_DIR=`${PKG_CONFIG} --variable=extensiondir libnautilus-extension`
	AC_SUBST([NAUTILUS_EXTENSION_DIR])
fi

AC_MSG_CHECKING([whether to build the Nemo extension])
AC_ARG_ENABLE([nemo],
	[AS_HELP_STRING([--enable-nemo], [build the Nemo extension])],
	[enable_nemo="${enableval}"], [enable_nemo="no"])
AC_MSG_RESULT(["${enable_nemo}"])
AM_CONDITIONAL([ENABLE_NEMO], [test "${enable_nemo}" = "yes"])

if test "${enable_nemo}" = "yes" ; then
	# Check for nemo
	PKG_CHECK_MODULES([NEMO], [libnemo-extension])
	AC_SUBST([NEMO_CFLAGS])
	AC_SUBST([NEMO_LIBS])
	NEMO_EXTENSION_DIR=`${PKG_CONFIG} --variable=extensiondir libnemo-extension`
	AC_SUBST([NEMO_EXTENSION_DIR])
fi

AC_MSG_CHECKING([whether to build the Thunar extension])
AC_ARG_ENABLE([thunar],
	[AS_HELP_STRING([--enable-thunar], [build the Thunar extension])],
	[enable_thunar="${enableval}"], [enable_thunar="no"])
AC_MSG_RESULT(["${enable_thunar}"])
AM_CONDITIONAL([ENABLE_THUNAR], [test "${enable_thunar}" = "yes"])

if test "${enable_thunar}" = "yes" ; then
	# Check for thunar
	PKG_CHECK_MODULES([THUNAR], [thunarx-2])
	AC_SUBST([THUNAR_CFLAGS])
	AC_SUBST([THUNAR_LIBS])
	THUNAR_EXTENSION_DIR=`${PKG_CONFIG} --variable=extensionsdir thunarx-2`
	AC_SUBST([THUNAR_EXTENSION_DIR])
fi

AM_CONDITIONAL([ENABLE_NAUTILUS_DATA],
	[test "${enable_nautilus}" = "yes" || test "${enable_nemo}" == "yes" || test "${enable_thunar}" = "yes"])
AM_CONDITIONAL([ENABLE_NLS], [test "${enable_nls}" != "no"])

if test "${enable_nls}" = "no" ; then
	AC_DEFINE([ENABLE_NLS], [0])
else
	GETTEXT_PACKAGE=nemo-extensions
	AC_SUBST([GETTEXT_PACKAGE])
	AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["${GETTEXT_PACKAGE}"], [Define the gettext package])
	IT_PROG_INTLTOOL([0.40.6])
	AM_GLIB_GNU_GETTEXT
fi

AC_DEFINE_UNQUOTED([G_LOG_DOMAIN], ["${PACKAGE_NAME}"], [Define the Glib log domain])

AC_DEFINE([HASH_FILE_BUFFER_SIZE], [131072], [Define file read buffer size (bytes)])
AC_DEFINE([HASH_FILE_REPORT_INTERVAL], [166], [Define progress report interval (milliseconds)])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([data/Makefile])
AC_CONFIG_FILES([data/nautilus/Makefile])
AC_CONFIG_FILES([po/Makefile.in])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/hash/Makefile])
AC_CONFIG_FILES([src/nautilus/Makefile])
AC_CONFIG_FILES([src/nemo/Makefile])
AC_CONFIG_FILES([src/thunar/Makefile])
AC_OUTPUT

echo
echo "Interfaces:"
echo "  gtkhash: ${enable_gtkhash} (gtk+-${with_gtk})"
echo "  gtkhash-nautilus: ${enable_nautilus}"
echo "  gtkhash-nemo: ${enable_nemo}"
echo "  gtkhash-thunar: ${enable_thunar}"

gcrypt_funcs="
	MD4 MD5
	SHA1 SHA224 SHA256 SHA384 SHA512
	RIPEMD160
	TIGER192
	WHIRLPOOL"
glib_checksums_funcs="
	MD5
	SHA1 SHA256"
internal_md6_funcs="
	MD6-224 MD6-256 MD6-384 MD6-512"
libcrypto_funcs="
	MD4 MD5
	SHA0 SHA1
	RIPEMD160
	MDC2"
linux_crypto_funcs="
	MD4 MD5
	SHA1 SHA224 SHA256 SHA384 SHA512
	RIPEMD128 RIPEMD160 RIPEMD256 RIPEMD320
	TIGER128 TIGER160 TIGER192
	WHIRLPOOL"
mhash_funcs="
	MD2 MD4 MD5
	SHA1 SHA224 SHA256 SHA384 SHA512
	RIPEMD128 RIPEMD160 RIPEMD256 RIPEMD320
	TIGER128 TIGER160 TIGER192
	WHIRLPOOL
	HAVAL128-3 HAVAL160-3 HAVAL192-3 HAVAL224-3 HAVAL256-3
	GOST
	SNEFRU128 SNEFRU256"
nettle_funcs="
	MD2 MD4 MD5
	SHA1 SHA224 SHA256 SHA384 SHA512
	SHA3-224 SHA3-256 SHA3-384 SHA3-512
	RIPEMD160
	GOST"
nss_funcs="
	MD2 MD5
	SHA1 SHA256 SHA384 SHA512"
mbedtls_funcs="
	MD2 MD4 MD5
	RIPEMD160
	SHA1 SHA224 SHA256 SHA384 SHA512"
zlib_funcs="
	CRC32
	ADLER32"
echo
echo "Supported Hash Functions:"
for func in \
	MD2 MD4 MD5 MD6-224 MD6-256 MD6-384 MD6-512 \
	SHA0 SHA1 SHA224 SHA256 SHA384 SHA512 \
	SHA3-224 SHA3-256 SHA3-384 SHA3-512 \
	RIPEMD128 RIPEMD160 RIPEMD256 RIPEMD320 \
	TIGER128 TIGER160 TIGER192 \
	WHIRLPOOL \
	HAVAL128-3 HAVAL160-3 HAVAL192-3 HAVAL224-3 HAVAL256-3 \
	GOST \
	SNEFRU128 SNEFRU256 \
	MDC2 \
	CRC32 \
	ADLER32
do
	printf "  %-14s" "${func}"
	enabled=""
	for lib in ${hash_libs} ; do
		if eval echo \${${lib}_funcs} | tr ' ' '\n' \
			| grep -q "^${func}$"
		then
			test -n "${enabled}" && printf ","
			printf "${lib}"
			enabled="yes"
		fi
	done
	test -n "${enabled}" && echo || echo "<disabled>"
done
