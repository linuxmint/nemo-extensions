nemo_extensiondir = $(NEMO_EXTENSIONS_DIR)

nemo_extension_LTLIBRARIES = libnemo-wipe.la

localedir = $(datadir)/locale

libnemo_wipe_la_SOURCES  = extension.c \
                               nw-compat.h \
                               nw-delete-operation.c \
                               nw-delete-operation.h \
                               nw-extension.c \
                               nw-extension.h \
                               nw-fill-operation.c \
                               nw-fill-operation.h \
                               nw-operation.c \
                               nw-operation.h \
                               nw-operation-manager.c \
                               nw-operation-manager.h \
                               nw-path-list.c \
                               nw-path-list.h \
                               nw-progress-dialog.c \
                               nw-progress-dialog.h \
                               nw-type-utils.h
libnemo_wipe_la_CPPFLAGS = -DG_LOG_DOMAIN=\"$(PACKAGE)\" \
                               -DLOCALEDIR=\"$(localedir)\"
libnemo_wipe_la_CFLAGS   = @GLIB_CFLAGS@ \
                               @GTK_CFLAGS@ \
                               @GCONF_CFLAGS@ \
                               @LIBNEMO_EXTENSION_CFLAGS@ \
                               @GSECUREDELETE_CFLAGS@ \
                               @GIO_UNIX_CFLAGS@
libnemo_wipe_la_LDFLAGS  = -module -avoid-version \
                               @GLIB_LIBS@ \
                               @GTK_LIBS@ \
                               @GCONF_LIBS@ \
                               @LIBNEMO_EXTENSION_LIBS@ \
                               @GSECUREDELETE_LIBS@ \
                               @GIO_UNIX_LIBS@
