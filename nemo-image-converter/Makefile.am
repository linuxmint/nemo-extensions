SUBDIRS = data src po

DISTCHECK_CONFIGURE_FLAGS = --with-nemodir='$${libdir}/nemo/extensions-2.0-distcheck'

ACLOCAL_AMFLAGS = -I m4

INTLTOOL = 			\
	intltool-extract.in 	\
	intltool-merge.in 	\
	intltool-update.in

EXTRA_DIST = 			\
	ChangeLog               \
	AUTHORS			\
	NEWS			\
	$(INTLTOOL)

DISTCLEANFILES = 		\
	intltool-extract	\
	intltool-merge		\
	intltool-update

