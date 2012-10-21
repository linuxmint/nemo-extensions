plugindir = $(libdir)/nemo-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        $(NEMO_SENDTO_CFLAGS)               \
        $(DBUS_CFLAGS)				\
        $(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libnstthunderbird.la

libnstthunderbird_la_SOURCES = thunderbird.c
libnstthunderbird_la_LDFLAGS = -module -avoid-version
libnstthunderbird_la_LIBADD = $(DBUS_LIBS) $(NEMO_SENDTO_LIBS)

