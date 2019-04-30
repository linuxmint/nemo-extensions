libnemo_share_la_CFLAGS =				\
	-Wall						\
	-DG_LOG_DOMAIN=\"Nemo-Share\"		\
    -DDATADIR=\"$(datadir)\"            \
    -DPKGDATADIR=\"$(pkgdatadir)\" \
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)                                  \
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(NEMO_SHARE_CFLAGS)

nemo_extensiondir=$(NEMO_EXTENSION_DIR)

nemo_extension_LTLIBRARIES=libnemo-share.la

libnemo_share_la_SOURCES =  \
	nemo-share.c        \
	nemo-share.h	\
	shares.c		\
	shares.h

libnemo_share_la_LDFLAGS = -module -avoid-version
libnemo_share_la_LIBADD  = $(NEMO_SHARE_LIBS)

sambahelperdir = $(datadir)/nemo-share

sambahelper_SCRIPTS = \
        install-samba \
        check-directory-permissions \
        $(NULL)

polkitdir = $(datadir)/polkit-1/actions
polkit_in_files = org.nemo.share.samba_install.policy.in
polkit_DATA = $(polkit_in_files:.policy.in=.policy)

org.nemo.share.samba_install.policy: org.nemo.share.samba_install.policy.in Makefile
	$(AM_V_GEN) sed -e "s|\@pkgdatadir\@|$(pkgdatadir)|" $< > $@

@INTLTOOL_POLICY_RULE@
@INTLTOOL_SERVER_RULE@

EXTRA_DIST =                    \
    $(polkit_in_files)      \
    $(NULL)
