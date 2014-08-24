AM_CPPFLAGS =						\
	-DG_LOG_DOMAIN=\"Nemo-Image-Converter\"	\
	-DDATADIR=\"$(datadir)\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)                                  \
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(NEMO_CFLAGS)

nemo_extensiondir=$(NEMO_EXTENSION_DIR)

nemo_extension_LTLIBRARIES=libnemo-image-converter.la

libnemo_image_converter_la_SOURCES =				\
	image-converter.c						\
	nemo-image-converter.c	nemo-image-converter.h	\
	nemo-image-resizer.c	nemo-image-resizer.h	\
	nemo-image-rotator.c	nemo-image-rotator.h

libnemo_image_converter_la_LDFLAGS = -module -avoid-version
libnemo_image_converter_la_LIBADD  = $(NEMO_LIBS)
