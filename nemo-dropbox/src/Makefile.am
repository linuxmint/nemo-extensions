INCLUDES =						\
	-I$(top_srcdir)					\
	-I$(top_builddir)

nemo_extensiondir=$(NEMO_EXTENSION_DIR)

nemo_extension_LTLIBRARIES=libnemo-dropbox.la

libnemo_dropbox_la_CFLAGS = 	                \
	-DDATADIR=\"$(datadir)\"					    \
	-DEMBLEMDIR=\"$(EMBLEM_DIR)\"					\
	-Wall                                           \
	$(WARN_CFLAGS)                                  \
	$(DISABLE_DEPRECATED_CFLAGS)					\
	$(NEMO_CFLAGS)                              \
	$(GLIB_CFLAGS)

if DEBUG
libnemo_dropbox_la_CFLAGS += -DND_DEBUG
else
libnemo_dropbox_la_CFLAGS += -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS
endif

libnemo_dropbox_la_SOURCES = \
	nemo-dropbox.c       \
	nemo-dropbox.h       \
	nemo-dropbox-hooks.h \
	nemo-dropbox-hooks.c \
	dropbox-command-client.h \
	dropbox-command-client.c \
	dropbox-client.c dropbox-client.h \
	g-util.h \
	async-io-coroutine.h \
	dropbox-client-util.c \
	dropbox-client-util.h \
	dropbox.c

libnemo_dropbox_la_LDFLAGS = -module -avoid-version
libnemo_dropbox_la_LIBADD  = $(NEMO_LIBS) $(GLIB_LIBS)
