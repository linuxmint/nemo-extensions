INCLUDES =						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DGETTEXT_PACKAGE=\""nemo-extensions"\" 	\
	-I$(top_srcdir)					\
	-I$(top_builddir)

nemo_extensiondir=$(NEMO_EXTENSION_DIR)

nemo_extension_LTLIBRARIES=libnemo-easytag.la

libnemo_easytag_la_CFLAGS = 	                \
	-DDATADIR=\"$(datadir)\"					    \
	-DLOCALEDIR=\"$(localedir)\"						\
	-DEMBLEMDIR=\"$(EMBLEM_DIR)\"					\
	-Wall                                           \
	$(WARN_CFLAGS)                                  \
	$(DISABLE_DEPRECATED_CFLAGS)					\
	$(NEMO_CFLAGS)                              \
	$(GLIB_CFLAGS)

if DEBUG
libnemo_easytag_la_CFLAGS += -DND_DEBUG
else
libnemo_easytag_la_CFLAGS += -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS
endif

libnemo_easytag_la_SOURCES = \
	nemo-easytag.c

libnemo_easytag_la_LDFLAGS = -module -avoid-version
libnemo_easytag_la_LIBADD  = $(NEMO_LIBS) $(GLIB_LIBS)
