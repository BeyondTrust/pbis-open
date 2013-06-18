# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------

DOXYGEN_SOURCE_DIR = text/OpenSOAP

SUBDIRS = text

DOXYGEN = @DOXYGEN@

DOXYFILE = Doxyfile

EXTRA_DIST = \
	${DOXYFILE}

DOCS_SRC_STAMP = .docs-src-stamp

output-dir:
	${mkinstalldirs} man/man3/OpenSOAP

docs-src:
	@if test "x${DOXYGEN_SOURCE_DIR}" != "x" \
	  && test -d ${DOXYGEN_SOURCE_DIR}; then \
	    (cd ${DOXYGEN_SOURCE_DIR} && ${MAKE} ${AM_MAKEFLAGS} $@ ) \
	fi
	@touch ${DOCS_SRC_STAMP}

${DOCS_SRC_STAMP}: docs-src

docs: ${DOXYFILE} ${DOCS_SRC_STAMP} output-dir
	@if test "x${DOXYGEN}" = "x"; then \
	  echo "Please install doxygen and ./configure execute at top directory"; \
	fi
	@${DOXYGEN}
	@rm -f ${DOCS_SRC_STAMP}

dist-hook: docs
	@cp -rf html ${distdir}/.
	@cp -rf man  ${distdir}/.
