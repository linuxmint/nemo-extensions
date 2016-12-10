# Initialization

AC_INIT([nemo-fileroller],3.2.1)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AC_PREFIX_DEFAULT(/usr)

AM_CONFIG_HEADER(config.h)

# not ready for i18n yet
#AC_PROG_INTLTOOL([0.29])

# Dependency checks
NEMO_REQUIRED=1.0.0
GLIB_REQUIRED=2.14.0

# Used programs
AC_PROG_CC
AM_PROG_LIBTOOL

# Check for pkg-config
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(NEMO, libnemo-extension >= $NEMO_REQUIRED)
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)

# Make dependency CFLAGS and LIBS available
AC_SUBST(NEMO_CFLAGS)
AC_SUBST(NEMO_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

# lol stolen from the automake manual
AC_ARG_ENABLE([debug],
[  --enable-debug    Turn on debugging],
[case "${enableval}" in
yes) debug=true ;;
no)  debug=false ;;
*) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],[debug=false])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])

AC_ARG_WITH(nemo-extension-dir,
              [AS_HELP_STRING([--with-nemo-extension-dir],
                    [specify the nemo extension directory])])

# Get nemo extensions directory
AC_MSG_CHECKING([for nemo extension directory])
if test -n "$with_nemo_extension_dir"; then
    NEMO_EXTENSION_DIR=$with_nemo_extension_dir
else
    NEMO_EXTENSION_DIR=`$PKG_CONFIG --variable=extensiondir libnemo-extension`
fi
if test -z "$NEMO_EXTENSION_DIR"; then
    NEMO_EXTENSION_DIR='${exec_prefix}/lib/nemo/extension-1.0'
fi

AC_MSG_RESULT([${NEMO_EXTENSION_DIR}])
AC_SUBST(NEMO_EXTENSION_DIR)

NEMO_VERSION=`$PKG_CONFIG --modversion libnemo-extension`
if test -n "$NEMO_VERSION"; then
    NEMO_VERSION_MAJOR=`echo -n $NEMO_VERSION | cut -d . -f 1`
    NEMO_VERSION_MINOR=`echo -n $NEMO_VERSION | cut -d . -f 2`
    NEMO_VERSION_MICRO=`echo -n $NEMO_VERSION | cut -d . -f 3`
fi

AC_DEFINE_UNQUOTED(NEMO_VERSION_MAJOR, [$NEMO_VERSION_MAJOR], [ nemo version major])
AC_DEFINE_UNQUOTED(NEMO_VERSION_MINOR, [$NEMO_VERSION_MINOR], [ nemo version minor])
AC_DEFINE_UNQUOTED(NEMO_VERSION_MICRO, [$NEMO_VERSION_MICRO], [ nemo version micro])

AC_CONFIG_FILES([
	Makefile 
	src/Makefile])

AC_OUTPUT
