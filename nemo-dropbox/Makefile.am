AUTOMAKE_OPTIONS = dist-bzip2 no-dist-gzip

bin_SCRIPTS = dropbox
CLEANFILES = $(bin_SCRIPTS) dropbox.1 dropbox.txt
EXTRA_DIST = dropbox.in serializeimages.py dropbox.txt.in docgen.py rst2man.py
man_MANS = dropbox.1

dropbox: dropbox.in serializeimages.py
	python serializeimages.py $(PACKAGE_VERSION) $(datadir)/applications < dropbox.in > dropbox
	chmod +x dropbox

dropbox.1: dropbox.txt.in dropbox docgen.py
	python docgen.py $(PACKAGE_VERSION) < dropbox.txt.in > dropbox.txt
	$(RST2MAN) dropbox.txt > dropbox.1

SUBDIRS = data src
