SUBDIRS = style

servicedir = $(datadir)/dbus-1/services
service_in_files = org.nemo.Preview.service.in
service_DATA = $(service_in_files:.service.in=.service)

org.nemo.Preview.service: org.nemo.Preview.service.in Makefile
	$(AM_V_GEN) sed -e "s|\@bindir\@|$(bindir)|" $< > $@

@INTLTOOL_XML_NOMERGE_RULE@

EXTRA_DIST = \
	$(service_in_files)

CLEANFILES = \
	$(service_DATA)

