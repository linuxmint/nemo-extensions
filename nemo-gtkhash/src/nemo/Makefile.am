nemodir = $(NEMO_EXTENSION_DIR)
nemo_LTLIBRARIES = libgtkhash-properties.la

libgtkhash_properties_la_SOURCES = \
	../nautilus/properties.c ../nautilus/properties.h \
	../nautilus/properties-hash.c ../nautilus/properties-hash.h \
	../nautilus/properties-list.c ../nautilus/properties-list.h \
	../nautilus/properties-prefs.c ../nautilus/properties-prefs.h

libgtkhash_properties_la_CPPFLAGS = \
	$(NEMO_CFLAGS) \
	-DDATADIR="\"$(pkgdatadir)/nautilus\"" \
	-DLOCALEDIR="\"$(localedir)\"" \
	-DIN_NEMO_EXTENSION=1

libgtkhash_properties_la_CFLAGS = $(GTKHASH_CFLAGS)

libgtkhash_properties_la_LDFLAGS = \
	-avoid-version \
	-export-symbols-regex ^nemo_module_ \
	-module \
	-shared

libgtkhash_properties_la_LIBADD = \
	../hash/hash.la $(NEMO_LIBS)
