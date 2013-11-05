nautilusdir = $(pkgdatadir)/nautilus
nautilus_DATA = gtkhash-properties.xml.gz

gsettings_SCHEMAS = org.nemo.extensions.gtkhash.gschema.xml
EXTRA_DIST = gtkhash-properties.xml $(gsettings_SCHEMAS)
CLEANFILES = $(nautilus_DATA)

gtkhash-properties.xml.gz: gtkhash-properties.xml
	$(AM_V_GEN) gzip -c9 $< > $@

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@
