AM_CPPFLAGS = \
    -I$(top_builddir)/config \
    -I$(top_srcdir)	\
    -L$(top_srcdir)/src \
    -DNEMO_PREVIEW_LIBDIR=\"$(libdir)\" \
    -DNEMO_PREVIEW_DATADIR=\"$(datadir)\" \
    -DNEMO_PREVIEW_PKGDATADIR=\"$(pkgdatadir)\" \
    -DNEMO_PREVIEW_TOP_SRCDIR=\"$(abs_top_srcdir)\" \
    -DNEMO_PREVIEW_BUILDDIR=\"$(abs_top_builddir)\"

girdir = $(pkgdatadir)/gir-1.0
typelibdir = $(pkglibdir)/girepository-1.0

BUILT_SOURCES =
EXTRA_DIST =
CLEANFILES =
pkglib_LTLIBRARIES =
INTROSPECTION_GIRS =
gir_DATA =
typelib_DATA =

# convenience command for doing Makefile variable substitutions in non-Makefile
# files (scripts, service files, etc.)
do_subst = sed -e 's|@abs_top_srcdir[@]|$(abs_top_srcdir)|g' \
               -e 's|@abs_top_builddir[@]|$(abs_top_builddir)|g' \
               -e 's|@localedir[@]|$(localedir)|g' \
               -e 's|@bindir[@]|$(bindir)|g' \
               -e 's|@libdir[@]|$(libdir)|g' \
               -e 's|@libexecdir[@]|$(libexecdir)|g' \
               -e 's|@pkglibdir[@]|$(pkglibdir)|g' \
               -e 's|@pkgdatadir[@]|$(pkgdatadir)|g' \
               -e 's|@have_unoconv[@]|$(HAVE_UNOCONV)|g'

include $(INTROSPECTION_MAKEFILE)

include Makefile-js.am
include Makefile-nemo-preview.am

gir_DATA += $(INTROSPECTION_GIRS)
typelib_DATA += $(gir_DATA:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)

libexec_PROGRAMS = nemo-preview-start

nemo_preview_start_SOURCES = main.c

nemo_preview_start_LDADD = \
    libnemo-preview-1.0.la \
    $(NEMO_PREVIEW_LIBS)
nemo_preview_start_CPPFLAGS = \
    -DGST_USE_UNSTABLE_API \
    $(NEMO_PREVIEW_CFLAGS) \
    $(AM_CPPFLAGS) \
    -DLOCALEDIR=\"$(datadir)/locale\"

bin_SCRIPTS = nemo-preview

nemo-preview: nemo-preview.in
	$(AM_V_GEN) $(do_subst) $< > $@
	chmod +x $@

CLEANFILES += nemo-preview
EXTRA_DIST += nemo-preview.in
