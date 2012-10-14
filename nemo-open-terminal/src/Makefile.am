INCLUDES =						\
	-DG_LOG_DOMAIN=\"Nemo-Open-Terminal\"	\
	-DDATADIR=\"$(datadir)\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)                                  \
	-Werror                                         \
	$(DISABLE_DEPRECATED)				\
	$(NEMO_CFLAGS)				\
	$(GCONF_CFLAGS)					\
	$(GNOMEDESKTOP_CFLAGS)

nemo_extensiondir=$(NEMO_EXTENSION_DIR)

nemo_extension_LTLIBRARIES=libnemo-open-terminal.la

libnemo_open_terminal_la_SOURCES = \
	eel-gnome-extensions.c       \
	eel-gnome-extensions.h       \
	nemo-open-terminal.h       \
	nemo-open-terminal.c       \
	nemo-open-terminal.h       \
	open-terminal.c

libnemo_open_terminal_la_LDFLAGS = -module -avoid-version
libnemo_open_terminal_la_LIBADD  = $(NEMO_LIBS) $(GCONF_LIBS) $(GNOMEDESKTOP_LIBS)
