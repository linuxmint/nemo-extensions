uidir = $(datadir)/nemo-seahorse/ui/
localedir = $(datadir)/locale

INCLUDES = -I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libseahorse \
	$(SEAHORSE_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDATA_DIR=\""$(datadir)"\" \
	-DSEAHORSE_UIDIR=\""$(uidir)"\" \
	-DLIBCRYPTUI_API_SUBJECT_TO_CHANGE

bin_PROGRAMS = seahorse-tool

BUILT_SOURCES = \
	seahorse-marshal.c seahorse-marshal.h

seahorse_tool_SOURCES = \
	seahorse-libdialogs.h \
	seahorse-notification.c \
	seahorse-operation.c seahorse-operation.h \
	seahorse-passphrase.c seahorse-passphrase.h \
	seahorse-pgp-operation.c seahorse-pgp-operation.h \
	seahorse-progress.c seahorse-progress.h \
	seahorse-secure-buffer.c seahorse-secure-buffer.h \
	seahorse-secure-memory.c seahorse-secure-memory.h \
	seahorse-tool.c seahorse-tool.h \
	seahorse-tool-files.c \
	seahorse-tool-progress.c \
	seahorse-util.c seahorse-util.h \
	seahorse-vfs-data.c seahorse-vfs-data.h \
	seahorse-widget.c seahorse-widget.h \
	$(BUILT_SOURCES)

seahorse-marshal.h: seahorse-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=seahorse_marshal > $@

seahorse-marshal.c: seahorse-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"seahorse-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=seahorse_marshal >> $@

ui_DATA = \
	seahorse-notify.xml \
	seahorse-progress.xml \
	seahorse-multi-encrypt.xml

seahorse_tool_LDADD = \
	$(SEAHORSE_LIBS) \
	$(LIBCRYPTUI_LIBS)

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = \
	seahorse-pgp-encrypted.desktop.in \
	seahorse-pgp-signature.desktop.in \
	seahorse-pgp-keys.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

man_MANS = seahorse-tool.1

EXTRA_DIST = \
	$(ui_DATA) \
	$(man_MANS) \
	$(mime_DATA) \
	$(mimeinfo_DATA) \
	$(desktop_in_files) \
	seahorse-pgp-encrypted.desktop.in.in \
	seahorse-pgp-keys.desktop.in.in \
	seahorse-pgp-signature.desktop.in.in \
	seahorse-marshal.list

CLEANFILES = *.bak \
	$(desktop_DATA) \
	seahorse-pgp-encrypted.desktop.in \
	seahorse-pgp-keys.desktop.in \
	seahorse-pgp-signature.desktop.in
