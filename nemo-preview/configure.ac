dnl -*- mode: m4 -*-
AC_PREREQ(2.63)
AC_INIT([nemo-preview],[2.2.0],[],[nemo-preview])

AC_CONFIG_HEADERS([config/config.h])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CANONICAL_TARGET

# require automake 1.10
AM_INIT_AUTOMAKE([1.10 dist-xz no-dist-gzip tar-ustar foreign])
AM_MAINTAINER_MODE([enable])

GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[The name of the gettext domain])
IT_PROG_INTLTOOL(0.40.0)
PKG_PROG_PKG_CONFIG([0.22])

GOBJECT_INTROSPECTION_REQUIRE([0.9.6])

## don't rerun to this point if we abort
AC_CACHE_SAVE

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_ISC_POSIX
AC_HEADER_STDC

# no stupid static libraries
AM_DISABLE_STATIC
# enable libtool
AC_PROG_LIBTOOL
# avoid libtool for LTCOMPILE, use it only to link

## don't rerun to this point if we abort
AC_CACHE_SAVE

GLIB_GSETTINGS

GJS_JS_DIR=`$PKG_CONFIG --variable=datadir cjs-1.0`/cjs-1.0
AC_SUBST(GJS_JS_DIR)

CLUTTER_MIN_VERSION=1.11.4
GLIB_MIN_VERSION=2.29.14
GTK_MIN_VERSION=3.4.0
CJS_MIN_VERSION=1.9.0
CLUTTER_GTK_MIN_VERSION=1.0.1
GOBJECT_INTROSPECTION_MIN_VERSION=0.9.6
GDK_PIXBUF_MIN_VERSION=2.23.0

PKG_CHECK_MODULES(NEMO_PREVIEW,
                  freetype2
                  glib-2.0 >= $GLIB_MIN_VERSION
                  gobject-introspection-1.0 >= $GOBJECT_INTROSPECTION_MIN_VERSION
                  cjs-1.0 >= $CJS_MIN_VERSION
                  gobject-2.0 >= $GLIB_MIN_VERSION
                  clutter-1.0 >= $CLUTTER_MIN_VERSION
                  clutter-x11-1.0 >= $CLUTTER_MIN_VERSION
                  gtk+-3.0 >= $GTK_MIN_VERSION
                  clutter-gtk-1.0 >= $CLUTTER_GTK_MIN_VERSION
                  clutter-gst-2.0
                  gstreamer-1.0
                  gstreamer-pbutils-1.0
                  gstreamer-tag-1.0
                  libmusicbrainz5
                  evince-document-3.0
                  evince-view-3.0
                  gtksourceview-3.0
                  webkitgtk-3.0)

GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_MKENUMS)

AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
data/style/Makefile
po/Makefile.in
])

AC_OUTPUT

dnl ==========================================================================
echo "

        nemo-preview $VERSION
        ==================

        prefix:    ${prefix}
        compiler:  ${CC}

        Now type 'make' to build $PACKAGE
"
