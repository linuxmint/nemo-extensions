SUBDIRS = hash

if ENABLE_NAUTILUS
SUBDIRS += nautilus
endif

if ENABLE_NEMO
SUBDIRS += nemo
endif

if ENABLE_THUNAR
SUBDIRS += thunar
endif

if ENABLE_GTKHASH

bin_PROGRAMS = gtkhash

gtkhash_SOURCES = \
	callbacks.c callbacks.h \
	gui.c gui.h \
	hash.c hash.h \
	list.c list.h \
	main.c main.h \
	prefs.c prefs.h

gtkhash_CPPFLAGS = \
	$(GTK_CFLAGS) \
	-DDATADIR="\"$(pkgdatadir)\"" \
	-DLOCALEDIR="\"$(localedir)\""

gtkhash_CFLAGS = $(GTKHASH_CFLAGS)

gtkhash_LDADD = hash/hash.la $(GTK_LIBS)

endif
