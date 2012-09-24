SUBDIRS = . $(PLUGINS)
DIST_SUBDIRS = $(ALL_PLUGINS)

NST_COMMON_SOURCES = nst-common.c nst-common.h

if HAVE_GIO
noinst_LTLIBRARIES = libnstcommon.la
libnstcommon_la_SOURCES = $(NST_COMMON_SOURCES)
libnstcommon_la_CFLAGS = $(GIO_CFLAGS)
libnstcommon_la_LIBADD = $(GIO_LIBS)
endif

EXTRA_DIST = $(NST_COMMON_SOURCES)
