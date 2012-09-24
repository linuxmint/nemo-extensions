plugindir = $(libdir)/nemo-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        -I$(srcdir)/../				\
        $(NEMO_SENDTO_CFLAGS)               \
	$(WARN_CFLAGS)				\
	$(DISABLE_DEPRECATED)			\
	$(GIO_CFLAGS)

plugin_LTLIBRARIES = libnstburn.la

libnstburn_la_SOURCES = nemo-burn.c
libnstburn_la_LDFLAGS = -module -avoid-version
libnstburn_la_LIBADD = $(GIO_LIBS) $(NEMO_SENDTO_LIBS) $(builddir)/../libnstcommon.la

