libnemo_share_la_CFLAGS =				\
	-Wall						\
	-DG_LOG_DOMAIN=\"Nemo-Share\"		\
	-DDATADIR=\"$(datadir)\"			\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)                                  \
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(NEMO_CFLAGS)				\
	$(GLIB_CFLAGS)

nemo_extensiondir=$(NEMO_EXTENSION_DIR)

nemo_extension_LTLIBRARIES=libnemo-share.la

libnemo_share_la_SOURCES =  \
	nemo-share.c        \
	nemo-share.h	\
	shares.c		\
	shares.h

libnemo_share_la_LDFLAGS = -module -avoid-version
libnemo_share_la_LIBADD  = $(GLIB_LIBS) $(NEMO_LIBS)
