SUBDIRS = plugins
INCLUDES =					\
	-DDATADIR=\"$(datadir)\"		\
	-DPLUGINDIR=\"$(libdir)/nemo-sendto/plugins\"	\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-DUIDIR=\""$(uidir)"\"		\
	-DLOCALEDIR="\"$(datadir)/locale\""	\
	$(NEMO_SENDTO_CFLAGS)		\
	$(NEMO_EXT_SENDTO_CFLAGS)		\
	$(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

nemo_sendto_includedir = $(includedir)/nemo-sendto/
nemo_sendto_include_HEADERS = nemo-sendto-plugin.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = nemo-sendto.pc

ui_DATA = nemo-sendto.ui
uidir = $(datadir)/nemo-sendto/ui

noinst_LTLIBRARIES = libnstplugin.la
libnstplugin_la_SOURCES =			\
	nemo-sendto-plugin.h		\
	nst-enum-types.c			\
	nst-enum-types.h
libnstplugin_la_LIBADD = $(NEMO_SENDTO_LIBS)

bin_PROGRAMS = nemo-sendto

nemo_sendto_SOURCES = nemo-sendto-command.c
nemo_sendto_LDADD = $(NEMO_SENDTO_LIBS) libnstplugin.la

nemo_extensiondir = $(NEMO_EXTENSION_DIR)
nemo_extension_LTLIBRARIES = libnemo-sendto.la
libnemo_sendto_la_SOURCES =		\
	nemo-nste.c			\
	nemo-nste.h			\
	nemo-sendto-module.c

libnemo_sendto_la_LDFLAGS = -module -avoid-version -no-undefined
libnemo_sendto_la_LIBADD  = $(NEMO_EXT_SENDTO_LIBS)

nst_headers_to_scan_for_enums = nemo-sendto-plugin.h
# Generate the enums source code, with glib-mkenums:
# This is based on the same Makefile.am stuff in pango:
nst_built_headers = nst-enum-types.h
nst_built_cfiles = nst-enum-types.c

nst-enum-types.h: $(nst_headers_to_scan_for_enums) Makefile
	$(AM_V_GEN) (cd $(srcdir) && glib-mkenums \
			--fhead "#ifndef __NST_ENUM_TYPES_H__\n#define __NST_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define NST_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
			--ftail "G_END_DECLS\n\n#endif /* __NST_ENUM_TYPES_H__ */" \
		$(nst_headers_to_scan_for_enums)) > $@

nst-enum-types.c: $(nst_headers_to_scan_for_enums) Makefile nst-enum-types.h
	$(AM_V_GEN) (cd $(srcdir) && glib-mkenums \
			--fhead "#include <nemo-sendto-plugin.h>\n" \
			--fhead "#include \"nst-enum-types.h\"\n" \
			--fhead "#include <glib-object.h>" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(nst_headers_to_scan_for_enums)) > $@

convertdir=$(datadir)/GConf/gsettings
convert_DATA = nemo-sendto-convert

gsettingsschema_in_files = org.nemo.Sendto.gschema.xml.in
gsettings_SCHEMAS = $(gsettingsschema_in_files:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = $(ui_DATA) nemo-sendto.pc.in $(convert_DATA) $(gsettingsschema_in_files)
CLEANFILES = $(nst_built_headers) $(nst_built_cfiles)
