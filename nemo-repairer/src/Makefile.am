INCLUDES = \
	-DGNOMELOCALEDIR=\""$(localedir)"\"   \
	$(WARN_CFLAGS)                        \
	$(DISABLE_DEPRECATED_CFLAGS)          \
	$(NEMO_CFLAGS)

nemo_extensiondir=$(NEMO_EXTENSION_DIR)
nemo_extension_LTLIBRARIES=libnemo-filename-repairer.la

libnemo_filename_repairer_la_SOURCES =    \
	filename-repairer.c                   \
	nemo-filename-repairer.c          \
	nemo-filename-repairer.h          \
	nemo-filename-repairer-i18n.h     \
	$(NULL)

libnemo_filename_repairer_la_LDFLAGS = -module -avoid-version
libnemo_filename_repairer_la_LIBADD  = $(NEMO_LIBS)

bin_PROGRAMS = nemo-filename-repairer

nemo_filename_repairer_SOURCES = \
	repairer.c \
	repair-dialog.h \
	repair-dialog.c \
	encoding-dialog.h \
	encoding-dialog.c \
	$(NULL)

nemo_filename_repairer_CFLAGS = \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	$(NEMO_CFLAGS) \
	$(NULL)

nemo_filename_repairer_LDADD = $(NEMO_LIBS)

pkgdata_DATA = \
	repair-dialog.ui \
	encoding-dialog.ui \
	$(NULL)

EXTRA_DIST = \
	$(pkgdata_DATA) \
	$(NULL)
