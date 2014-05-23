# Initially based on NemoActions configure & build system.

AC_PREREQ([2.64])
AC_INIT([nemo-wipe], [0.2.1], [ban@herbesfolles.org])
AC_CONFIG_SRCDIR([nemo-wipe/nw-extension.h])
AC_CONFIG_AUX_DIR([build/aux])
AC_CONFIG_MACRO_DIR([build/m4])
AM_INIT_AUTOMAKE([1.11.1 -Wall -Wno-portability foreign])
AC_CONFIG_HEADERS([config.h])

AM_SILENT_RULES([yes])

# Checks for programs.
LT_PREREQ([2.2.6])
LT_INIT([disable-static])
AC_PROG_CC
AM_PROG_CC_C_O
IT_PROG_INTLTOOL
PKG_PROG_PKG_CONFIG([0.9])

GNOME_DOC_INIT

# Checks for libraries.
# 2.6 for gtk_message_dialog_format_secondary_text()
# We use some newer functions, but we (should) only do so if available.
GTK_REQUIRED=2.6
# 2.10 to have the slice allocator (may be dropped if needed, but its probably a
# acceptable dependency).
# We use g_dngettext() (a 2.18 feature), but only conditionally if present.
GLIB_REQUIRED=2.10
# 2.4 for g_type_class_add_private()
GOBJECT_REQUIRED=2.4.0
GCONF_REQUIRED=2.0
NEMO_EXTENSION=2.0
GSECUREDELETE_REQUIRED=0.1
GIO_UNIX_REQUIRED=2.0

# choose whether to use GTK2 or GTK3
AC_ARG_WITH([gtk3],
            [AS_HELP_STRING([--with-gtk3],
                            [whether to build with GTK2 or GTK3 [[default=auto]]])],
            [with_gtk3="$withval"],
            [with_gtk3=check])
# use GTK3 by default but if not found
AC_MSG_CHECKING([for GTK package to use])
gtk_pkg="gtk+-3.0"
AS_IF([test "x$with_gtk3" = xcheck],
      [PKG_CHECK_EXISTS([$gtk_pkg >= ${GTK_REQUIRED}],
                        [],
                        [gtk_pkg="gtk+-2.0"])],
      [test "x$with_gtk3" = xno],
      [gtk_pkg="gtk+-2.0"])
AC_MSG_RESULT([$gtk_pkg])

# mandatory packages
PKG_CHECK_MODULES([GLIB],                   glib-2.0              >= ${GLIB_REQUIRED})
PKG_CHECK_MODULES([GTK],                    $gtk_pkg              >= ${GTK_REQUIRED})
PKG_CHECK_MODULES([LIBNEMO_EXTENSION],  libnemo-extension >= ${NEMO_EXTENSION})
PKG_CHECK_MODULES([GSECUREDELETE],          gsecuredelete         >= ${GSECUREDELETE_REQUIRED})
# GConf is mandatory, but only for Nemo 2.x
AS_IF([test "$gtk_pkg" = "gtk+-2.0"],
      [PKG_CHECK_MODULES([GCONF], gconf-2.0 >= ${GCONF_REQUIRED})
       AC_DEFINE([HAVE_GCONF], [1], [Whether we have GConf])])

# optional packages
HAVE_GIO_UNIX=0
PKG_CHECK_MODULES([GIO_UNIX], gio-unix-2.0 >= ${GIO_UNIX_REQUIRED},
                  [ HAVE_GIO_UNIX=1
                    AC_SUBST([GIO_UNIX_CFLAGS])
                    AC_SUBST([GIO_UNIX_LIBS]) ],
                  [ AC_MSG_WARN([GIO-UNIX not found. Some UNIX mount points might not be detected correctly. The error was: ${GIO_UNIX_PKG_ERRORS}])])
AC_DEFINE_UNQUOTED([HAVE_GIO_UNIX], [${HAVE_GIO_UNIX}], [Whether we have gio-unix])

# i18n
GETTEXT_PACKAGE="${PACKAGE}"
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["${GETTEXT_PACKAGE}"], [GNU gettext package])

AM_GLIB_GNU_GETTEXT

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# when working in a test environment, nemo is typically installed in
# a non-standard location ; lets specify this here
NA_NEMO_EXTDIR

# Checks for library functions.
# If we are here we're sure we have libnemo-extension, but we need this for
# the check below to work (for the lib to be found).
AC_CHECK_LIB([nemo-extension], [nemo_menu_provider_get_file_items])
AC_CHECK_FUNCS([nemo_file_info_get_location])

# Output
AC_CONFIG_FILES([Makefile
                 po/Makefile.in
                 nemo-wipe/Makefile
                 help/Makefile])
AC_OUTPUT
