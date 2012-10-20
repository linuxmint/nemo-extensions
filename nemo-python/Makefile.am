ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src examples

if ENABLE_GTK_DOC
SUBDIRS += docs
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = nemo-python.pc

EXTRA_DIST = nemo-python.pc.in

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc
