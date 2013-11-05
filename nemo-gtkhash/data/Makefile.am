SUBDIRS =

if ENABLE_NAUTILUS_DATA
SUBDIRS += nautilus
endif

if ENABLE_GTKHASH
pkgdata_DATA = gtkhash.xml.gz
gsettings_SCHEMAS = app.gtkhash.gschema.xml
EXTRA_DIST = gtkhash.xml $(gsettings_SCHEMAS)
CLEANFILES = $(pkgdata_DATA)
endif

gtkhash.xml.gz: gtkhash.xml
	$(AM_V_GEN) gzip -c9 $< > $@

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@
