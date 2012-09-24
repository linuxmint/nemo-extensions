plugindir = $(libdir)/nemo-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        $(NEMO_SENDTO_CFLAGS)               \
	$(UPNP_CFLAGS)				\
	$(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libnstupnp.la

libnstupnp_la_SOURCES = upnp.c
libnstupnp_la_LDFLAGS = -module -avoid-version
libnstupnp_la_LIBADD = $(UPNP_LIBS) $(NEMO_SENDTO_LIBS)

