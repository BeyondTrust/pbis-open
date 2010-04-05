MKDIR_P = @MKDIR_P@

if HAVE_DOXYGEN

if HAVE_DOXYGEN_PDF
DOXYGEN_PDF_TARGET=doxygen-pdf
DOXYGEN_PDF_INSTALL_TARGET=doxygen-pdf-install
else
DOXYGEN_PDF_TARGET=
DOXYGEN_PDF_INSTALL_TARGET=
endif

DOXYGEN_DEPS = \
	Doxyfile.in \
	@top_srcdir@/include/lw/threadpool.h

doxygen-html: $(DOXYGEN_DEPS)
	$(MKDIR_P) doxygen-html
	( cat Doxyfile; echo GENERATE_HTML=yes ) | $(DOXYGEN) -

doxygen-latex: $(DOXYGEN_DEPS)
	$(MKDIR_P) doxygen-latex
	( cat Doxyfile; echo GENERATE_LATEX=yes ) | $(DOXYGEN) -

lsass.pdf: doxygen-latex
	$(MAKE) -C doxygen-latex pdf
	cp doxygen-latex/refman.pdf lsass.pdf

doxygen-pdf: lsass.pdf

doxygen-html-install: doxygen-html
	$(MKDIR_P) -m 755 $(DESTDIR)$(docdir)/html
	$(INSTALL_DATA) $</* $(DESTDIR)$(docdir)/html

doxygen-pdf-install: lsass.pdf
	$(MKDIR_P) -m 755 $(DESTDIR)$(docdir)/pdf
	$(INSTALL_DATA) $< $(DESTDIR)$(docdir)/pdf

all-local: doxygen-html $(DOXYGEN_PDF_TARGET)

install-data-local: doxygen-html-install $(DOXYGEN_PDF_INSTALL)

.PHONY: doxygen-pdf doxygen-html-install doxygen-pdf-install

endif
