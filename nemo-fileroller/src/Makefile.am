INCLUDES =						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DGETTEXT_PACKAGE=\""file-roller"\" 	\
	-I$(top_srcdir)					\
	-I$(top_builddir)

nemo_extensiondir=$(NEMO_EXTENSION_DIR)

nemo_extension_LTLIBRARIES=libnemo-fileroller.la

libnemo_fileroller_la_CFLAGS = 	                \
	-DDATADIR=\"$(datadir)\"					    \
	-DEMBLEMDIR=\"$(EMBLEM_DIR)\"					\
	-Wall                                           \
	$(WARN_CFLAGS)                                  \
	$(DISABLE_DEPRECATED_CFLAGS)					\
	$(NEMO_CFLAGS)                              \
	$(GLIB_CFLAGS)

if DEBUG
libnemo_fileroller_la_CFLAGS += -DND_DEBUG
else
libnemo_fileroller_la_CFLAGS += -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS
endif

libnemo_fileroller_la_SOURCES = \
	fileroller-module.c       \
	nemo-fileroller.h       \
	nemo-fileroller.c

libnemo_fileroller_la_LDFLAGS = -module -avoid-version
libnemo_fileroller_la_LIBADD  = $(NEMO_LIBS) $(GLIB_LIBS)
