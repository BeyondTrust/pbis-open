INSTALL_DIR = ${MKDIR_P} -m 755

DOCBOOK_DEPS = Doxyfile.in @top_srcdir@/include/moonunit/*.h
DOC_TARGETS =
DOC_INSTALL_TARGETS =
DOCBOOK_XML = index.xml moonunit.xml moonunit-lt.xml moonunit-stub.xml
EXTRA_DIST = $(DOCBOOK_XML) main.doxy doxygen-html doxygen-man docbook-html docbook-man

if HAVE_DOXYGEN
DOC_TARGETS += doxygen-doc
DOC_INSTALL_TARGETS += doxygen-install

doxygen-html: $(DOCBOOK_DEPS)
	$(MKDIR_P) doxygen-html
	( cat Doxyfile; echo GENERATE_HTML=yes ) | $(DOXYGEN) -

doxygen-man: $(DOCBOOOK_DEPS)
	$(MKDIR_P) doxygen-man
	( cat Doxyfile; echo GENERATE_MAN=yes ) | $(DOXYGEN) -

doxygen-doc: doxygen-html doxygen-man
endif

doxygen-html-install: doxygen-html
	$(INSTALL_DIR) $(DESTDIR)$(htmldir)/api
	$(INSTALL_DATA) $</* $(DESTDIR)$(htmldir)/api

doxygen-man-install: doxygen-man
	$(INSTALL_DIR) $(DESTDIR)$(htmldir)/api
	$(INSTALL_DIR) $(DESTDIR)$(mandir)/man1
	$(INSTALL_DIR) $(DESTDIR)$(mandir)/man3mu
	$(INSTALL_DATA) $</man3mu/*.3mu $(DESTDIR)$(mandir)/man3mu

doxygen-install: doxygen-html-install doxygen-man-install


if HAVE_DOCBOOK
DOC_TARGETS += docbook-doc
DOC_INSTALL_TARGETS += docbook-install

docbook-html: $(DOCBOOK_XML)
	$(MKDIR_P) docbook-html
	$(XSLTPROC) --output docbook-html/ --param citerefentry.link 1 --param use.id.as.filename 1 --xinclude docbook-html.xsl @srcdir@/index.xml

docbook-man: $(DOCKBOOK_XML)
	$(MKDIR_P) docbook-man/man1
	$(XSLTPROC) --output docbook-man/man1/ --xinclude docbook-man.xsl @srcdir@/index.xml

docbook-doc: docbook-html docbook-man
endif

docbook-html-install: docbook-html
	$(INSTALL_DIR) $(DESTDIR)$(htmldir)/tools
	$(INSTALL_DATA) $</* $(DESTDIR)$(htmldir)/tools

docbook-man-install: docbook-man
	$(INSTALL_DIR) $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) $</man1/* $(DESTDIR)$(mandir)/man1

docbook-install: docbook-html-install docbook-man-install

uninstall-local: doxygen-man
	rm -rf $(DESTDIR)$(htmldir)
	for file in doxygen-man/man1/*; do rm -f $(DESTDIR)$(mandir)/man1/`basename $$file`; done
	for file in doxygen-man/man3mu/*; do rm -f $(DESTDIR)$(mandir)/man3mu/`basename $$file`; done

distclean-local:
	rm -rf doxygen-html doxygen-man docbook-html docbook-man

all-local: $(DOC_TARGETS)

install-data-local: $(DOC_INSTALL_TARGETS)

.PHONY: doxygen-doc doxygen-install doxygen-man-install doxygen-html-install
.PHONY: docbook-doc docbook-install docbook-man-install docbook-html-install