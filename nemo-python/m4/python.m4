## this one is commonly used with AM_PATH_PYTHONDIR ...
dnl AM_CHECK_PYMOD(MODNAME [,SYMBOL [,ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]]])
dnl Check if a module containing a given symbol is visible to python.
AC_DEFUN([AM_CHECK_PYMOD],
[AC_REQUIRE([AM_PATH_PYTHON])
py_mod_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
AC_MSG_CHECKING(for ifelse([$2],[],,[$2 in ])python module $1)
AC_CACHE_VAL(py_cv_mod_$py_mod_var, [
ifelse([$2],[], [prog="
import sys
try:
        import $1
except ImportError:
        sys.exit(1)
except:
        sys.exit(0)
sys.exit(0)"], [prog="
import $1
$1.$2"])
if $PYTHON -c "$prog" 1>&AC_FD_CC 2>&AC_FD_CC
  then
    eval "py_cv_mod_$py_mod_var=yes"
  else
    eval "py_cv_mod_$py_mod_var=no"
  fi
])
py_val=`eval "echo \`echo '$py_cv_mod_'$py_mod_var\`"`
if test "x$py_val" != xno; then
  AC_MSG_RESULT(yes)
  ifelse([$3], [],, [$3
])dnl
else
  AC_MSG_RESULT(no)
  ifelse([$4], [],, [$4
])dnl
fi
])

dnl a macro to check for ability to create python extensions
dnl  AM_CHECK_PYTHON_HEADERS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_INCLUDES
AC_DEFUN([AM_CHECK_PYTHON_HEADERS],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_MSG_CHECKING(for headers required to compile python extensions)
dnl deduce PYTHON_INCLUDES
py_prefix=`$PYTHON -c "import sys; print(sys.prefix)"`
py_exec_prefix=`$PYTHON -c "import sys; print(sys.exec_prefix)"`
PYTHON_INCLUDES="-I${py_prefix}/include/python${PYTHON_VERSION}"
if test "$py_prefix" != "$py_exec_prefix"; then
  PYTHON_INCLUDES="$PYTHON_INCLUDES -I${py_exec_prefix}/include/python${PYTHON_VERSION}"
fi
AC_SUBST(PYTHON_INCLUDES)
dnl check if the headers exist:
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
AC_TRY_CPP([#include <Python.h>],dnl
[AC_MSG_RESULT(found)
$1],dnl
[AC_MSG_RESULT(not found)
$2])
CPPFLAGS="$save_CPPFLAGS"
])

# The AC_MULTILIB macro was extracted and modified from
# gettext-0.15's AC_LIB_PREPARE_MULTILIB macro in the lib-prefix.m4 file
# so that the correct paths can be used for 64-bit libraries.
#
dnl Copyright (C) 2001-2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl From Bruno Haible.

dnl AC_MULTILIB creates a variable libdirsuffix, containing
dnl the suffix of the libdir, either "" or "64".
dnl Only do this if the given enable parameter is "yes".
AC_DEFUN([AC_MULTILIB],
[
  dnl There is no formal standard regarding lib and lib64. The current
  dnl practice is that on a system supporting 32-bit and 64-bit instruction
  dnl sets or ABIs, 64-bit libraries go under $prefix/lib64 and 32-bit
  dnl libraries go under $prefix/lib. We determine the compiler's default
  dnl mode by looking at the compiler's library search path. If at least
  dnl of its elements ends in /lib64 or points to a directory whose absolute
  dnl pathname ends in /lib64, we assume a 64-bit ABI. Otherwise we use the
  dnl default, namely "lib".
  enable_lib64="$1"
  libdirsuffix="/i386-linux-gnu/"
  searchpath=`(LC_ALL=C $CC -print-search-dirs) 2>/dev/null | sed -n -e 's,^libraries: ,,p' | sed -e 's,^=,,'`
  if test "$enable_lib64" = "yes" -a -n "$searchpath"; then
    save_IFS="${IFS=    }"; IFS=":"
    for searchdir in $searchpath; do
      if test -d "$searchdir"; then
        case "$searchdir" in
          */lib64/ | */lib64 ) libdirsuffix=64 ;;
          *) searchdir=`cd "$searchdir" && pwd`
             case "$searchdir" in
               */lib64 ) libdirsuffix=64 ;;
             esac ;;
        esac
        case "$searchdir" in
          /lib/x86_64-linux-gnu/ | /lib/x86_64-linux-gnu ) libdirsuffix=/x86_64-linux-gnu ;;
          *) searchdir=`cd "$searchdir" && pwd`
             case "$searchdir" in
               /lib/x86_64-linux-gnu ) libdirsuffix=/x86_64-linux-gnu ;;
             esac ;;
        esac
      fi
    done
    IFS="$save_IFS"
  fi
  AC_SUBST(libdirsuffix)
])

dnl a macro to check for ability to embed python
dnl  AM_CHECK_PYTHON_LIBS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_LIBS
AC_DEFUN([AM_CHECK_PYTHON_LIBS],
[AC_REQUIRE([AM_CHECK_PYTHON_HEADERS])
AC_MSG_CHECKING(for libraries required to embed python)
dnl deduce PYTHON_LIBS
py_exec_prefix=`$PYTHON -c "import sys; print(sys.exec_prefix)"`
AC_MULTILIB(yes)

if test "x$PYTHON_LIBS" = x; then
	PYTHON_LIBS="-L${py_prefix}/lib${libdirsuffix} -lpython${PYTHON_VERSION}"
fi

if test "x$PYTHON_LIB_LOC" = x; then
	PYTHON_LIB_LOC="${py_prefix}/lib${libdirsuffix}" 
fi
AC_SUBST(PYTHON_LIBS)
AC_SUBST(PYTHON_LIB_LOC)
dnl check if the headers exist:
save_LIBS="$LIBS"
LIBS="$LIBS $PYTHON_LIBS"
AC_TRY_LINK_FUNC(Py_Initialize, dnl
         [LIBS="$save_LIBS"; AC_MSG_RESULT(yes); $1], dnl
         [LIBS="$save_LIBS"; AC_MSG_RESULT(no); $2])

])
