ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = src po data

EXTRA_DIST = \
    autogen.sh \
    COPYING \
    NEWS

stale-files-clean-local:
	-$(RM) $(abs_top_builddir)/*.la $(abs_top_builddir)/*.gir $(abs_top_builddir)/*.typelib

MAINTAINERCLEANFILES = \
    INSTALL \
    aclocal.m4 \
    config.log \
    config.status \
    configure
