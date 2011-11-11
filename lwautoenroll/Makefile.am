#
# Copyright (C) Likewise Software.  All rights reserved.
#
# Autotool configuration

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

# General configuration

AM_CFLAGS = @LWAUTOENROLL_CFLAGS@
AM_CPPFLAGS = @LWAUTOENROLL_CPPFLAGS@ -I$(top_srcdir)/include
MKDIR_P = @MKDIR_P@

# includes

lwautoenroll_includedir = $(includedir)/lwautoenroll
lwautoenroll_include_HEADERS = include/lwautoenroll/lwautoenroll.h

# libraries

lib_LTLIBRARIES = lib/liblwautoenroll.la

lib_liblwautoenroll_la_SOURCES = \
	lib/curl_util.c \
	lib/daemon_util.c \
	lib/ldap_util.c \
	lib/log_util.c \
	lib/request.c \
	lib/soap_util.c \
	lib/ssl_util.c \
	lib/template.c \
	lib/x509_util.c
lib_liblwautoenroll_la_LIBADD = \
	-llsaclient \
	-llwnetclientapi \
	-llwadvapi \
	-lcrypto \
	-lldap \
	-lcurl \
	-lOpenSOAPClient

# programs

sbin_PROGRAMS = lwcertd/lwcertd

lwcertd_lwcertd_SOURCES = \
	lwcertd/cert.c \
	lwcertd/main.c
lwcertd_lwcertd_LDADD = \
	lib/liblwautoenroll.la \
	-lpopt

# registry files
registry_DATA = config/lwcertd.reg
registrydir="${datadir}/config"

# tests

bin_PROGRAMS = test/request

test_request_SOURCES = test/request.c
test_request_LDADD = lib/liblwautoenroll.la

# documentation

if HAVE_DOXYGEN

if HAVE_DOXYGEN_PDF
DOXYGEN_PDF_TARGET = doxygen-pdf
DOXYGEN_PDF_INSTALL_TARGET = doxygen-pdf-install
else
DOXYGEN_PDF_TARGET =
DOXYGEN_PDF_INSTALL_TARGET =
endif

DOXYGEN_HEADER_DEPS = \
	$(top_srcdir)/include/lwautoenroll/lwautoenroll.h

DOXYGEN_DEPS = \
	doc/Doxyfile.in \
	$(DOXYGEN_HEADER_DEPS)

doc/doxygen-html: $(DOXYGEN_DEPS)
	$(MKDIR_P) $@ && \
	cd doc && ( cat Doxyfile; echo GENERATE_HTML=yes ) | $(DOXYGEN) - && \
	touch doxygen-html

doxygen-html: doc/doxygen-html

doc/doxygen-latex: $(DOXYGEN_DEPS)
	$(MKDIR_P) $@ && \
	cd doc && ( cat Doxyfile; echo GENERATE_LATEX=yes ) | $(DOXYGEN) - && \
	touch doxygen-latex

doxygen-latex: doc/doxygen-latex

doc/lwautoenroll.pdf: doc/doxygen-latex
	$(MAKE) -C doc/doxygen-latex pdf
	cp doc/doxygen-latex/refman.pdf doc/lwautoenroll.pdf

doxygen-pdf: doc/lwautoenroll.pdf

doxygen-html-install: doc/doxygen-html
	$(MKDIR_P) -m 755 $(DESTDIR)$(docdir)/html
	$(INSTALL_DATA) $</* $(DESTDIR)$(docdir)/html

doxygen-pdf-install: doc/lwautoenroll.pdf
	$(MKDIR_P) -m 755 $(DESTDIR)$(docdir)/pdf
	$(INSTALL_DATA) $< $(DESTDIR)$(docdir)/pdf

all-local: doxygen-html $(DOXYGEN_PDF_TARGET)

install-data-local: doxygen-html-install $(DOXYGEN_PDF_INSTALL_TARGET)

endif

# Make options

.DELETE_ON_ERROR:
