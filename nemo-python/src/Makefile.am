INCLUDES =						\
	-DG_LOG_DOMAIN=\"Nemo-Python\"			\
	-DDATADIR=\"$(datadir)\"			\
	-DLIBDIR=\"$(libdir)\"				\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(NEMO_PYTHON_CFLAGS)				\
	-DPYTHON_SO=\"$(PYTHON_SO)\"     		\
	$(PYTHON_INCLUDES)

nemo_extensiondir=$(NEMO_EXTENSION_DIR)

nemo_extension_LTLIBRARIES = libnemo-python.la

libnemo_python_la_SOURCES =	\
	nemo-python.c		\
	nemo-python.h		\
	nemo-python-object.c	\
	nemo-python-object.h

libnemo_python_la_LDFLAGS = -module -avoid-version
libnemo_python_la_LIBADD  = $(NEMO_PYTHON_LIBS) $(PYTHON_LIBS)
