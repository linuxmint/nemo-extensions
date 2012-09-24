plugindir = $(libdir)/nemo-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        $(NEMO_SENDTO_CFLAGS)               \
	$(NST_EBOOK_CFLAGS)			\
	$(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

MARSHALFILES = econtactentry-marshal.c econtactentry-marshal.h
BUILT_SOURCES = $(MARSHALFILES)

econtactentry-marshal.c: econtactentry-marshal.h
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=econtactentry_marshal $(srcdir)/econtactentry-marshal.list --header --body > econtactentry-marshal.c )
econtactentry-marshal.h: econtactentry-marshal.list
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=econtactentry_marshal $(srcdir)/econtactentry-marshal.list --header > econtactentry-marshal.h )

CLA_FILES = e-contact-entry.h e-contact-entry.c econtactentry-marshal.list
CLADIR = $(srcdir)/../../../../contact-lookup-applet/src/
regenerate-built-sources:
	EGGFILES="$(CLA_FILES) econtactentry-marshal.list" EGGDIR="$(CLADIR)" $(srcdir)/update-from-egg.sh || true

plugin_LTLIBRARIES = libnstevolution.la

libnstevolution_la_SOURCES = evolution.c $(CLA_FILES) $(MARSHALFILES)
libnstevolution_la_LDFLAGS = -module -avoid-version
libnstevolution_la_LIBADD = $(NST_EBOOK_LIBS) $(NEMO_SENDTO_LIBS)

EXTRA_DIST = $(CLA_FILES)
CLEANFILES = econtactentry-marshal.c econtactentry-marshal.h

