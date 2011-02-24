# Autotool configuration

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

# General configuration

AM_CFLAGS = @LWMSG_CFLAGS@
AM_CPPFLAGS = @LWMSG_CPPFLAGS@ -I$(top_srcdir)/include -I$(top_builddir)/include
MKDIR_P = @MKDIR_P@

lib_LTLIBRARIES =
bin_PROGRAMS =

# include

noinst_HEADERS = \
	include/call-private.h \
	include/data-private.h \
	include/assoc-private.h \
	include/connection-private.h \
	include/context-private.h \
	include/convert-private.h \
	include/protocol-private.h \
	include/status-private.h \
	include/test-private.h \
	include/type-private.h \
	include/util-private.h \
	include/xnet-private.h \
	include/session-private.h \
	include/mt19937ar.h \
	include/peer-private.h \
	include/archive-private.h \
	include/task-private.h

lwmsg_includedir = $(includedir)/lwmsg

lwmsg_include_HEADERS = \
	include/lwmsg/call.h \
	include/lwmsg/message.h \
	include/lwmsg/data.h \
	include/lwmsg/assoc.h \
	include/lwmsg/context.h \
	include/lwmsg/lwmsg.h \
	include/lwmsg/time.h \
	include/lwmsg/buffer.h \
	include/lwmsg/connection.h \
	include/lwmsg/protocol.h \
	include/lwmsg/status.h \
	include/lwmsg/type.h \
	include/lwmsg/peer.h \
	include/lwmsg/security.h \
	include/lwmsg/common.h \
	include/lwmsg/session.h \
	include/lwmsg/archive.h

# src

lib_LTLIBRARIES += src/liblwmsg_nothr.la src/liblwmsg.la

src_liblwmsg_nothr_la_SOURCES = \
	src/assoc.c \
	src/assoc-marshal.c \
	src/assoc-call.c \
	src/assoc-session.c \
	src/call.c \
	src/convert.c \
	src/connection.c \
	src/connection-buffer.c \
	src/connection-state.c \
	src/connection-wire.c \
	src/connection-security.c \
	src/connection-marshal.c \
	src/message.c \
	src/protocol.c \
	src/data-context.c \
	src/data-graph.c \
	src/data-marshal.c \
	src/data-unmarshal.c \
	src/data-print.c \
	src/type.c \
	src/type-iterate.c \
	src/type-rep.c \
	src/type-print.c \
	src/status.c \
	src/context.c \
	src/util.c \
	src/time.c \
	src/xnet.c \
	src/security.c \
	src/mt19937ar.c \
	src/session.c \
	src/archive.c \
	src/archive-format.c

src_liblwmsg_nothr_la_LIBADD = @LIB_XNET@ @LIB_DL@ @LIB_ICONV@ @LIB_RT@
src_liblwmsg_nothr_la_LDFLAGS = $(AM_LDFLAGS) -export-symbols $(top_srcdir)/src/liblwmsg_nothr.sym

src_liblwmsg_la_SOURCES = \
	src/peer.c \
	src/peer-task.c \
	src/peer-call.c \
	src/peer-session.c

src_liblwmsg_la_LIBADD = src/liblwmsg_nothr.la @LIB_PTHREAD@ -llwbase
src_liblwmsg_la_LDFLAGS = $(AM_LDFLAGS) -export-symbols $(top_srcdir)/src/liblwmsg.sym

bin_PROGRAMS += lwma

lwma_SOURCES = src/lwma-main.c
lwma_LDADD = src/liblwmsg_nothr.la

# test

if HAVE_MOONUNIT

lib_LTLIBRARIES += test/lwmsg_mu.la

TEST_SOURCES = \
	test/test-main.c \
	test/test-convert.c \
	test/test-marshal.c \
	test/test-assoc.c \
	test/test-client-server.c \
	test/test-async.c \
	test/test-archive.c \
	test/test-task.c

BUILT_SOURCES = test/stub.c

test_lwmsg_mu_la_SOURCES = $(TEST_SOURCES) test/stub.c
test_lwmsg_mu_la_CFLAGS = -shared $(AM_CFLAGS)
test_lwmsg_mu_la_LIBADD = @LIB_moonunit@ $(top_builddir)/src/liblwmsg.la
test_lwmsg_mu_la_LDFLAGS = -shared -module -avoid-version

test/stub.c: $(TEST_SOURCES)
	$(MKDIR_P) test
	$(MOONUNIT_STUB) -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS) $(test_lwmsg_mu_la_CPPFLAGS)" $^

endif

# doc

doc_DATA = doc/README.devel

if HAVE_DOXYGEN

if HAVE_DOXYGEN_PDF
DOXYGEN_PDF_TARGET = doxygen-pdf
DOXYGEN_PDF_INSTALL_TARGET = doxygen-pdf-install
else
DOXYGEN_PDF_TARGET =
DOXYGEN_PDF_INSTALL_TARGET =
endif

DOXYGEN_SOURCE_DEPS = \
	$(top_srcdir)/include/lwmsg/call.h \
	$(top_srcdir)/include/lwmsg/assoc.h \
	$(top_srcdir)/include/lwmsg/buffer.h \
	$(top_srcdir)/include/lwmsg/common.h \
	$(top_srcdir)/include/lwmsg/connection.h \
	$(top_srcdir)/include/lwmsg/context.h \
	$(top_srcdir)/include/lwmsg/data.h \
	$(top_srcdir)/include/lwmsg/lwmsg.h \
	$(top_srcdir)/include/lwmsg/protocol.h \
	$(top_srcdir)/include/lwmsg/security.h \
	$(top_srcdir)/include/lwmsg/peer.h \
	$(top_srcdir)/include/lwmsg/session.h \
	$(top_srcdir)/include/lwmsg/status.h \
	$(top_srcdir)/include/lwmsg/time.h \
	$(top_srcdir)/include/lwmsg/type.h \
	$(top_srcdir)/include/lwmsg/archive.h \
	$(top_srcdir)/include/lwmsg/message.h \
	$(top_srcdir)/include/archive-private.h \
	$(top_srcdir)/include/assoc-private.h \
	$(top_srcdir)/include/call-private.h \
	$(top_srcdir)/include/connection-private.h \
	$(top_srcdir)/include/context-private.h \
	$(top_srcdir)/include/convert-private.h \
	$(top_srcdir)/include/data-private.h \
	$(top_srcdir)/include/mt19937ar.h \
	$(top_srcdir)/include/protocol-private.h \
	$(top_srcdir)/include/security-private.h \
	$(top_srcdir)/include/peer-private.h \
	$(top_srcdir)/include/session-private.h \
	$(top_srcdir)/include/status-private.h \
	$(top_srcdir)/include/test-private.h \
	$(top_srcdir)/include/type-private.h \
	$(top_srcdir)/include/util-private.h \
	$(top_srcdir)/include/xnet-private.h \
	$(top_srcdir)/example/fserv/include/protocol.h \
	$(top_srcdir)/example/fserv/src/protocol.c \
	$(top_srcdir)/example/fserv/src/fserv-client.c \
	$(top_srcdir)/example/fserv/src/fserver.c

DOXYGEN_DEPS = \
	doc/Doxyfile.in \
	doc/main.doxy \
	doc/arch.doxy \
	doc/arch_rep.doxy \
	doc/arch_sess.doxy \
	doc/example_fserv.doxy \
	doc/example_fserv_layout.doxy \
	doc/example_fserv_source.doxy \
	$(DOXYGEN_SOURCE_DEPS)

doc/doxygen-html: $(DOXYGEN_DEPS) doc/doxy-header.html doc/doxy-footer.html doc/doxygen.css
	$(MKDIR_P) $@ && \
	cd doc && ( cat Doxyfile; echo GENERATE_HTML=yes ) | $(DOXYGEN) - && \
	touch doxygen-html

doxygen-html: doc/doxygen-html

doc/doxygen-latex: $(DOXYGEN_DEPS)
	$(MKDIR_P) $@ && \
	cd doc && ( cat Doxyfile; echo GENERATE_LATEX=yes ) | $(DOXYGEN) - && \
	touch doxygen-latex

doxygen-latex: doc/doxygen-latex

doc/lwmsg.pdf: doc/doxygen-latex
	$(MAKE) -C doc/doxygen-latex pdf
	cp doc/doxygen-latex/refman.pdf doc/lwmsg.pdf

doxygen-pdf: doc/lwmsg.pdf

doxygen-html-install: doc/doxygen-html
	$(MKDIR_P) -m 755 $(DESTDIR)$(docdir)/html
	$(INSTALL_DATA) $</* $(DESTDIR)$(docdir)/html

doxygen-pdf-install: doc/lwmsg.pdf
	$(MKDIR_P) -m 755 $(DESTDIR)$(docdir)/pdf
	$(INSTALL_DATA) $< $(DESTDIR)$(docdir)/pdf

all-local: doxygen-html $(DOXYGEN_PDF_TARGET)

install-data-local: doxygen-html-install $(DOXYGEN_PDF_INSTALL_TARGET)

endif

# example

EXTRA_DIST = example/fserv

# Make options

.PHONY: doxygen-html doxygen-latex doxygen-pdf doxygen-html-install doxygen-pdf-install
.DELETE_ON_ERROR:
