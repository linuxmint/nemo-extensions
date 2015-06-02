AC_INIT([nemo-filename-repairer], [2.6.0], [choe.hwanjin@gmail.com], [nemo-filename-repairer])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_DISABLE_STATIC
IT_PROG_INTLTOOL([0.35.0])
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=nemo-filename-repairer
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

NEMO_REQUIRED=2.2.2

PKG_CHECK_MODULES(NEMO, [
    libnemo-extension >= $NEMO_REQUIRED
])
AC_SUBST(NEMO_CFLAGS)
AC_SUBST(NEMO_LIBS)

AC_ARG_WITH(nemo-extension-dir,
	      [AS_HELP_STRING([--with-nemo-extension-dir],
		    [specify the nemo extension directory])])

AC_MSG_CHECKING([for nemo extension directory])
if test -n "$with_nemo_extension_dir"; then
    NEMO_EXTENSION_DIR=$with_nemo_extension_dir
else
    NEMO_EXTENSION_DIR=`$PKG_CONFIG --variable=extensiondir libnemo-extension`
fi
if test -z "$NEMO_EXTENSION_DIR"; then
    NEMO_EXTENSION_DIR='${exec_prefix}/lib/nemo/extensions-2.0'
fi

AC_MSG_RESULT([${NEMO_EXTENSION_DIR}])
AC_SUBST(NEMO_EXTENSION_DIR)

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 po/Makefile.in])

AC_OUTPUT
