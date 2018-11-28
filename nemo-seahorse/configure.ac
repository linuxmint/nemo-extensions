AC_PREREQ([2.63])
AC_INIT([nemo-seahorse], [4.0.0],
        [https://github.com/linuxmint/nemo-extensions/issues],
        [nemo-seahorse])

# -----------------------------------------------------------------------------

AM_INIT_AUTOMAKE([1.11 tar-ustar foreign -Wno-portability])
AM_SANITY_CHECK
AM_CONFIG_HEADER(config.h)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_MAINTAINER_MODE([enable])
AC_CONFIG_MACRO_DIR([m4])

# -----------------------------------------------------------------------------
# Basic tools

AC_GNU_SOURCE
AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_DISABLE_STATIC
AM_PROG_LIBTOOL
IT_PROG_INTLTOOL([0.35.0])
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

AC_SYS_LARGEFILE
AC_TYPE_OFF_T
AC_FUNC_FSEEKO

GETTEXT_PACKAGE=nemo-extensions
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of the gettext domain.])

# -----------------------------------------------------------------------------
# GLib and GTK+ stuff

GLIB_GSETTINGS
AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.0.0 gio-2.0)

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

SEAHORSE_CFLAGS="$SEAHORSE_CFLAGS $GTK_CFLAGS"
SEAHORSE_LIBS="$SEAHORSE_LIBS $GTK_LIBS"

# -----------------------------------------------------------------------------
# GnuPG and GPGME

AC_ARG_ENABLE(gpg-check,
	AC_HELP_STRING([--disable-gpg-check], [check GPG version (default is yes)]),
	DO_CHECK=$enableval, DO_CHECK=yes)

if test	"$DO_CHECK" = "yes"; then
	accepted_versions="1.2 1.4 2.0 2.1 2.2"
	AC_PATH_PROGS(GNUPG, [gpg gpg2], no)
	ok="no"
	if test "$GNUPG" != "no"; then
		AC_MSG_CHECKING(for appropriate GnuPG version)
		gnupg_version=`$GNUPG --version | grep gpg`
		major=`echo $gnupg_version | \
			sed 's/^gpg (GnuPG) \([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\1/'`
		minor=`echo $gnupg_version | \
			sed 's/^gpg (GnuPG) \([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\2/'`
		micro=`echo $gnupg_version | \
			sed 's/^gpg (GnuPG) \([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\3/'`

		for ver in $accepted_versions; do
			if test "$ver" = "$major.$minor"; then
				AC_DEFINE_UNQUOTED(GPG_MAJOR, [$major], [GPG Major Version])
				AC_DEFINE_UNQUOTED(GPG_MINOR, [$minor], [GPG Minor Version])
				ok="yes"
				break
			fi
		done
	fi

	if test "$ok" = "yes"; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_ERROR([Appropriate version of GnuPG not found. Please install one of versions: $accepted_versions])
	fi
fi

ok="no"
min_gpgme_version=1.2.0
AC_PATH_PROG(GPGME_CONFIG, gpgme-config, "failed")
if test $GPGME_CONFIG != "failed" ; then
	AC_MSG_CHECKING(for GPGME - version >= $min_gpgme_version)
	req_major=`echo $min_gpgme_version | \
		sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\1/'`
	req_minor=`echo $min_gpgme_version | \
		sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\2/'`
	req_micro=`echo $min_gpgme_version | \
		sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\3/'`
	gpgme_config_version=`$GPGME_CONFIG --version`
	major=`echo $gpgme_config_version | \
		sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\1/'`
	minor=`echo $gpgme_config_version | \
		sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\2/'`
	micro=`echo $gpgme_config_version | \
		sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\3/'`

	if test "$major" -eq "$req_major"; then
		if test "$minor" -ge "$req_minor"; then
			if test "$micro" -ge "$req_micro"; then
				ok="yes"
			fi
		fi
	fi
fi

if test $ok = "yes"; then
	GPGME_CFLAGS=`$GPGME_CONFIG --cflags`
	GPGME_LIBS=`$GPGME_CONFIG --libs`
	AC_MSG_RESULT(yes)
else
	AC_MSG_ERROR(GPGME $min_gpgme_version or later needed)
fi

SEAHORSE_CFLAGS="$SEAHORSE_CFLAGS $GPGME_CFLAGS"
SEAHORSE_LIBS="$SEAHORSE_LIBS $GPGME_LIBS"

# -----------------------------------------------------------------------------
# Nemo

PKG_CHECK_MODULES(NEMO, [libnemo-extension >= 1.0.0 glib-2.0 >= 2.37.3])

AC_SUBST(NEMO_CFLAGS)
AC_SUBST(NEMO_LIBS)

AC_ARG_WITH([nemo-ext-dir],
            [AC_HELP_STRING([--with-nemo-ext-dir=DIR],
                            [directory to nemo extension config])],
            [], [with_nemo_ext_dir=""])

if test -z "$with_nemo_ext_dir"; then
	NEMO_EXTENSION_DIR=`$PKG_CONFIG --variable=extensiondir libnemo-extension`
else
	NEMO_EXTENSION_DIR="$with_nemo_ext_dir"
fi
AC_SUBST(NEMO_EXTENSION_DIR)

# -----------------------------------------------------------------------------
# Keyring

PKG_CHECK_MODULES([GNOME_KEYRING], [gnome-keyring-1])
SEAHORSE_CFLAGS="$SEAHORSE_CFLAGS $GNOME_KEYRING_CFLAGS"
SEAHORSE_LIBS="$SEAHORSE_LIBS $GNOME_KEYRING_LIBS"

# -----------------------------------------------------------------------------
# DBus

DBUS_GLIB_REQUIRED=0.78

PKG_CHECK_MODULES([DBUS], [dbus-glib-1 >= $DBUS_GLIB_REQUIRED], [enable_dbus=yes],[enable_dbus=no])
if test "$enable_dbus" = "no"; then
	AC_MSG_ERROR([dbus or dbus-glib-1 bindings not found.])
fi

AC_SUBST([DBUS_CFLAGS])
AC_SUBST([DBUS_LIBS])

SEAHORSE_CFLAGS="$SEAHORSE_CFLAGS $DBUS_CFLAGS"
SEAHORSE_LIBS="$SEAHORSE_LIBS $DBUS_LIBS"

# -----------------------------------------------------------------------------
# Libcryptui

PKG_CHECK_MODULES([LIBCRYPTUI], cryptui-0.0)

SEAHORSE_CFLAGS="$SEAHORSE_CFLAGS $LIBCRYPTUI_CFLAGS"
SEAHORSE_LIBS="$SEAHORSE_LIBS $LIBCRYPTUI_LIBS"

# -----------------------------------------------------------------------------
# Gcr

PKG_CHECK_MODULES(GCR, gcr-3 >= 3.4.0)
SEAHORSE_CFLAGS="$SEAHORSE_CFLAGS $GCR_CFLAGS"
SEAHORSE_LIBS="$SEAHORSE_LIBS $GCR_LIBS"

# -----------------------------------------------------------------------------
# Libnotify

LIBNOTIFY_REQUIRED=0.7.0

# Allow libnotify to be disabled
AC_ARG_ENABLE(libnotify,
              AC_HELP_STRING([--disable-libnotify],
              [Don't compile in libnotify support]))

if test "x$enable_libnotify" = "xno"; then
	echo "disabling libnotify support"
else

	PKG_CHECK_MODULES(LIBNOTIFY, [libnotify >= $LIBNOTIFY_REQUIRED],
	                  enable_libnotify=yes, enable_libnotify=no)
	if test "$enable_libnotify" = "yes"; then
		AC_DEFINE(HAVE_LIBNOTIFY, 1, [libnotify available])
		SEAHORSE_CFLAGS="$SEAHORSE_CFLAGS $LIBNOTIFY_CFLAGS"
		SEAHORSE_LIBS="$SEAHORSE_LIBS $LIBNOTIFY_LIBS"
	fi
fi

# --------------------------------------------------------------------
# Compilation and linking options
#

AC_MSG_CHECKING([for debug mode])
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug=no/default/yes],
              [Turn on or off debugging]))

if test "$enable_debug" != "no"; then
	AC_DEFINE_UNQUOTED(WITH_DEBUG, 1, [Print debug output])
	AC_DEFINE_UNQUOTED(_DEBUG, 1, [In debug mode])
	CFLAGS="$CFLAGS -g"
fi

if test "$enable_debug" = "yes"; then
	debug_status="yes (-g, -O0, debug output, testable)"
	CFLAGS="$CFLAGS -O0"
elif test "$enable_debug" = "no"; then
	debug_status="no (no debug output, not testable, G_DISABLE_ASSERT)"
	AC_DEFINE_UNQUOTED(G_DISABLE_ASSERT, 1, [Disable glib assertions])
else
	debug_status="default (-g, debug output, testable)"
fi

AC_MSG_RESULT($debug_status)

AC_MSG_CHECKING(for more warnings)
if test "$GCC" = "yes"; then
	AC_MSG_RESULT(yes)
	CFLAGS="\
	-Wall \
	-Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
	-Wnested-externs -Wpointer-arith \
	-Wcast-align -Wsign-compare \
	$CFLAGS"

	# Disable -Werror for now, because of issues with
	# strange gtk-doc programs not building cleanly.
	# -Werror \

	for option in -Wno-strict-aliasing -Wno-sign-compare; do
		SAVE_CFLAGS="$CFLAGS"
		CFLAGS="$CFLAGS $option"
		AC_MSG_CHECKING([whether gcc understands $option])
		AC_TRY_COMPILE([], [],
			has_option=yes,
			has_option=no,)
		if test $has_option = no; then
			CFLAGS="$SAVE_CFLAGS"
		fi
		AC_MSG_RESULT($has_option)
		unset has_option
		unset SAVE_CFLAGS
	done
	unset option
else
	AC_MSG_RESULT(no)
fi

AC_ARG_ENABLE(strict, [
               AS_HELP_STRING([--enable-strict], [Strict code compilation])
             ])

AC_MSG_CHECKING([build strict])

if test "$enable_strict" = "yes"; then
	CFLAGS="$CFLAGS -Werror \
		-DGTK_DISABLE_DEPRECATED \
		-DGDK_DISABLE_DEPRECATED \
		-DG_DISABLE_DEPRECATED \
		-DGDK_PIXBUF_DISABLE_DEPRECATED"
	AC_DEFINE_UNQUOTED(WITH_STRICT, 1, [More strict checks])
	strict_status="yes (-Werror, thorough tests, fatals, no deprecations)"
else
	strict_status="no (quick tests, non-fatal warnings)"
fi

AC_MSG_RESULT($strict_status)

# ----------------------------------------------------------------------

AC_SUBST(SEAHORSE_CFLAGS)
AC_SUBST(SEAHORSE_LIBS)

AC_OUTPUT([
Makefile
data/Makefile
po/Makefile.in
tool/Makefile
tool/nemo-seahorse-pgp-encrypted.desktop.in
tool/nemo-seahorse-pgp-keys.desktop.in
tool/nemo-seahorse-pgp-signature.desktop.in
nemo-ext/Makefile
])

echo "
GnuPG Version:           $gnupg_version
GPGME Version:           $gpgme_config_version
Notification Support:    $enable_libnotify
"
