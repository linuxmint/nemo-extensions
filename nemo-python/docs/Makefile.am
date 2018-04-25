BUILDDIR = $(top_builddir)/docs

REF_VERSION = $(VERSION)
FULL_VERSION = $(VERSION)

FIXXREF = $(srcdir)/xsl/fixxref.py
HTML_STYLE = $(srcdir)/xsl/ref-html-style.xsl

XML_FILES = 							\
	reference/nemo-python-ref.xml			\
	reference/nemo-python-overview.xml			\
	reference/nemo-python-overview-example.xml			\
	reference/nemo-python-overview-methods.xml			\
	reference/nemo-python-class-reference.xml			\
	reference/nemo-python-provider-reference.xml			\
	reference/nemo-python-column.xml   \
	reference/nemo-python-column-provider.xml   \
	reference/nemo-python-menu.xml  \
	reference/nemo-python-menu-item.xml \
	reference/nemo-python-menu-provider.xml \
	reference/nemo-python-property-page.xml \
	reference/nemo-python-property-page-provider.xml \
	reference/nemo-python-location-widget-provider.xml \
	reference/nemo-python-file-info.xml \
	reference/nemo-python-info-provider.xml \
	reference/nemo-python-enum-reference.xml \
	reference/nemo-python-operation-result.xml

HTMLdir = $(datadir)/gtk-doc/html/nemo-python
HTML_FILES =						\
	html/index.html					\
	html/index.sgml					\
	html/nemo-python-overview.html \
	html/nemo-python-overview-example.html \
	html/nemo-python-overview-methods.html \
	html/nemo-python-class-reference.html \
	html/nemo-python-provider-reference.html \
	html/class-nemo-python-column.html \
	html/class-nemo-python-column-provider.html \
	html/class-nemo-python-menu.html  \
	html/class-nemo-python-menu-item.html  \
	html/class-nemo-python-menu-provider.html  \
	html/class-nemo-python-property-page.html  \
	html/class-nemo-python-property-page-provider.html  \
	html/class-nemo-python-location-widget-provider.html  \
	html/class-nemo-python-file-info.html  \
	html/class-nemo-python-info-provider.html  \
	html/nemo-python-enum-reference.html \
	html/enum-nemo-python-operation-result.html  \
	html/nemo-python.devhelp
HTML_DATA = $(HTMLFILES)

XSL_FILES =             \
	xsl/common.xsl      \
	xsl/devhelp.xsl     \
	xsl/html.xsl        \
	xsl/pdf-style.xsl   \
	xsl/pdf.xsl         \
	xsl/style.css
XSL_DATA = $(XSL_FILES) $(FIXXREF)
XSLdir = $(BUILDDIR)/xsl

CSS_FILES = xsl/style.css
CSSdir = $(HTMLdir)
CSS_DATA = $(CSS_FILES)

BUILT_SOURCES = 		\
	$(HTML_FILES)

CLEANFILES = 			\
	nemo-python-ref.*	\
	$(HTML_FILES)

EXTRA_DIST = 		\
	$(XML_FILES)     \
	$(XSL_FILES)    \
	$(FIXXREF)      \
	$(HTML_STYLE)

REFERENCE_DEPS = 	\
	reference 	\
	$(XML_FILES)    \
	$(XSL_FILES)    \
	$(FIXXREF)

if ENABLE_GTK_DOC

$(HTML_FILES): $(REFERENCE_DEPS)
	xsltproc --nonet --xinclude -o $(BUILDDIR)/html/ \
		 --path $(BUILDDIR)/reference:$(srcdir)/reference \
		 --stringparam gtkdoc.bookname "nemo-python" \
		 --stringparam gtkdoc.version ${REF_VERSION} \
		 $(HTML_STYLE) $(srcdir)/reference/nemo-python-ref.xml
	$(FIXXREF) -i xsl $(BUILDDIR)/html 
	touch $@

endif
