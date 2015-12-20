AC_PREREQ(2.63)

AC_INIT([nemo-python], [3.0.1],
	[https://github.com/linuxmint/nemo-extensions/issues], [nemo-python])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_SUBST([PACKAGE_NAME], ["$PACKAGE_NAME"])
AC_SUBST([PACKAGE_VERSION], ["$PACKAGE_VERSION"])

AM_INIT_AUTOMAKE

NEMO_REQUIRED=1.0.3

AC_PROG_CC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

dnl Check for pkgconfig first
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, yes, no)
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

dnl Give error and exit if we dont have pkgconfig
if test "x$HAVE_PKGCONFIG" = "xno"; then
  AC_MSG_ERROR([you need to have pkgconfig installed !])
fi

dnl Check for gtk-doc
GTK_DOC_CHECK(1.9)

AC_PREFIX_DEFAULT([$(pkg-config --variable=prefix libnemo-extension || echo /usr)])

dnl **************************************************
dnl * Check for Python
dnl **************************************************
AM_PATH_PYTHON([3.3])
PKG_CHECK_MODULES([PYTHON], [python-${PYTHON_VERSION}])
PYTHON_LIB_LOC="`pkg-config python-${PYTHON_VERSION} --variable=libdir`"
# current python3 builds miss to set the libdir variable, try old one then
if test "$PYTHON_LIB_LOC" == "" ; then
  PYTHON_LIB_LOC="`pkg-config python2 --variable=libdir`"
fi
py_prefix=`$PYTHON -c "import sys; print(sys.prefix)"`
py_exec_prefix=`$PYTHON -c "import sys; print(sys.exec_prefix)"`
PYTHON_INCLUDES="-I${py_prefix}/include/python${PYTHON_VERSION}"
if test "$py_prefix" != "$py_exec_prefix"; then
  PYTHON_INCLUDES="$PYTHON_INCLUDES -I${py_exec_prefix}/include/python${PYTHON_VERSION}"
fi
AC_SUBST(PYTHON_INCLUDES)
AC_SUBST(PYTHON_LIBS)
AC_SUBST(PYTHON_CFLAGS)
AC_SUBST(PYTHON_LIB_LOC)

if test "`pkg-config --variable=datadir pygobject-3.0`" != "" ; then
    PYGOBJECT_VERSION=pygobject-3.0
    PYGOBJECT_MAJOR_VERSION=3
    PYGOBJECT_MINOR_VERSION=0
    PYGOBJECT_MICRO_VERSION=0
else
    PYGOBJECT_VERSION=pygobject-2.0
    PYGOBJECT_MAJOR_VERSION=2
    PYGOBJECT_MINOR_VERSION=28
    PYGOBJECT_MICRO_VERSION=2
fi

PYGOBJECT_REQUIRED=$PYGOBJECT_MAJOR_VERSION.$PYGOBJECT_MINOR_VERSION.$PYGOBJECT_MICRO_VERSION
AC_DEFINE_UNQUOTED(PYGOBJECT_MAJOR_VERSION,[$PYGOBJECT_MAJOR_VERSION], [PyGObject major version.])
AC_DEFINE_UNQUOTED(PYGOBJECT_MINOR_VERSION,[$PYGOBJECT_MINOR_VERSION], [PyGObject minor version.])
AC_DEFINE_UNQUOTED(PYGOBJECT_MICRO_VERSION,[$PYGOBJECT_MICRO_VERSION], [PyGObject micro version.])

PKG_CHECK_MODULES(NEMO_PYTHON, [$PYGOBJECT_VERSION >= $PYGOBJECT_REQUIRED
                                   libnemo-extension >= $NEMO_REQUIRED])

PYGOBJECT_DATADIR=`$PKG_CONFIG --variable=datadir $PYGOBJECT_VERSION`
AC_SUBST(PYGOBJECT_DATADIR)

PYGOBJECT_PYGDOCS="`$PKG_CONFIG --variable=pygdocs $PYGOBJECT_VERSION`"
AC_SUBST(PYGOBJECT_PYGDOCS)

PYGOBJECT_FIXXREF="$PYTHON `$PKG_CONFIG --variable=fixxref $PYGOBJECT_VERSION`"
AC_SUBST(PYGOBJECT_FIXXREF)

NEMO_LIBDIR=`$PKG_CONFIG --variable=libdir libnemo-extension`
AC_SUBST(NEMO_LIBDIR)
AC_DEFINE_UNQUOTED(NEMO_LIBDIR, "$NEMO_LIBDIR", [Nemo libdir])

dnl ---------------------------------
dnl Get nemo extensions directory
dnl ---------------------------------
NEMO_EXTENSION_DIR=`$PKG_CONFIG --variable=extensiondir libnemo-extension`
AC_SUBST(NEMO_EXTENSION_DIR)
AC_DEFINE_UNQUOTED(NEMO_EXTENSION_DIR, "$NEMO_EXTENSION_DIR", [Nemo' extensions directory])

dnl ---------------------------------
dnl Check for gtk-doc dependencies
dnl ---------------------------------
if test "${enable_gtk_doc}" != no; then
    dnl Check for xsltproc
    AC_PATH_PROG([XSLTPROC], [xsltproc])
    if test -z "$XSLTPROC"; then
        echo " disabling generation of docs"
        enable_gtk_doc=no
    fi
fi

AC_OUTPUT([
  Makefile
  examples/Makefile
  src/Makefile
  nemo-python.pc
  docs/Makefile
  docs/reference/entities.docbook
])

echo
echo " nemo-python $VERSION"
echo
echo "        Nemo Prefix: ${prefix}"
echo "     Python Version: ${PYTHON_VERSION}"
echo "  PyGObject Version: ${PYGOBJECT_VERSION}"
echo "      Documentation: ${enable_gtk_doc}"
echo
