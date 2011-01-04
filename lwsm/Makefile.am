# Autotool configuration

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

# General configuration

AM_CFLAGS = @LWSM_CFLAGS@
AM_CPPFLAGS = @LWSM_CPPFLAGS@ -I$(top_srcdir)/include
MKDIR_P = @MKDIR_P@

# includes

noinst_HEADERS = \
	include/common.h \
	include/client.h \
	include/server.h \
	common/includes.h \
	client/includes.h \
	server/includes.h
lwsm_includedir = $(includedir)/lwsm
lwsm_include_HEADERS = include/lwsm/lwsm.h

# libraries

lib_LTLIBRARIES = common/liblwsmcommon.la client/liblwsm.la server/liblwsmloader.la

common_liblwsmcommon_la_SOURCES = \
	common/protocol.c \
	common/util.c
common_liblwsmcommon_la_LIBADD = @LIB_LWMSG@ @LIB_LWADVAPI@ @LIB_LWBASE@

client_liblwsm_la_SOURCES = \
	client/client.c \
	client/call.c
client_liblwsm_la_LIBADD = $(top_builddir)/common/liblwsmcommon.la @LIB_PTHREAD@
client_liblwsm_la_LDFLAGS = $(AM_LDFLAGS) -export-symbols $(top_srcdir)/client/liblwsm.sym

server_liblwsmloader_la_SOURCES = server/loader.c
server_liblwsmloader_la_LIBADD = $(top_builddir)/common/liblwsmcommon.la @LIB_PTHREAD@ @LIB_DL@
server_liblwsmloader_la_LDFLAGS = $(AM_LDFLAGS) -export-symbols $(top_srcdir)/server/liblwsmloader.sym

# loaders

LOADER_CFLAGS = -shared
LOADER_LDFLAGS = -shared -module -avoid-version
loader_LTLIBRARIES = server/executable.la server/driver.la server/stub.la

server_executable_la_SOURCES = server/executable.c
server_executable_la_CFLAGS = $(AM_CFLAGS) $(LOADER_CFLAGS)
server_executable_la_LIBADD = $(top_builddir)/server/liblwsmloader.la
server_executable_la_LDFLAGS = $(AM_LDFLAGS) $(LOADER_LDFLAGS)

server_driver_la_SOURCES = server/driver.c
server_driver_la_CFLAGS = $(AM_CFLAGS) $(LOADER_CFLAGS)
server_driver_la_LIBADD = \
    $(top_builddir)/server/liblwsmloader.la \
    $(top_builddir)/common/liblwsmcommon.la \
    -llwioclient \
    @LIB_PTHREAD@
server_driver_la_LDFLAGS = $(AM_LDFLAGS) $(LOADER_LDFLAGS)

server_stub_la_SOURCES = server/stub.c
server_stub_la_CFLAGS = $(AM_CFLAGS) $(LOADER_CFLAGS)
server_stub_la_LIBADD = $(top_builddir)/server/liblwsmloader.la
server_stub_la_LDFLAGS = $(AM_LDFLAGS) $(LOADER_LDFLAGS)

# programs

sbin_PROGRAMS = server/lwsmd

server_lwsmd_SOURCES = \
	server/table.c \
	server/api.c \
	server/dispatch.c \
	server/registry.c \
	server/bootstrap.c \
	server/logger.c \
	server/main.c
server_lwsmd_LDADD = \
	$(top_builddir)/common/liblwsmcommon.la \
	$(top_builddir)/server/liblwsmloader.la \
	-llwmsg -llwmsg_nothr \
	-lregclient -llwadvapi -llwadvapi_nothr -llwbase -llwbase_nothr \
	@LIB_PTHREAD@

bin_PROGRAMS = client/lwsm

client_lwsm_SOURCES = client/main.c
client_lwsm_LDADD = \
	$(top_builddir)/client/liblwsm.la \
	$(top_builddir)/common/liblwsmcommon.la \
	-llwadvapi -llwadvapi_nothr @LIB_PTHREAD@

# documentation

doc_DATA = doc/README.devel

if HAVE_DOXYGEN

if HAVE_DOXYGEN_PDF
DOXYGEN_PDF_TARGET = doxygen-pdf
DOXYGEN_PDF_INSTALL_TARGET = doxygen-pdf-install
else
DOXYGEN_PDF_TARGET =
DOXYGEN_PDF_INSTALL_TARGET =
endif

DOXYGEN_HEADER_DEPS = \
	$(top_srcdir)/include/lwsm/lwsm.h

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

doc/lwsm.pdf: doc/doxygen-latex
	$(MAKE) -C doc/doxygen-latex pdf
	cp doc/doxygen-latex/refman.pdf doc/lwsm.pdf

doxygen-pdf: doc/lwsm.pdf

doxygen-html-install: doc/doxygen-html
	$(MKDIR_P) -m 755 $(DESTDIR)$(docdir)/html
	$(INSTALL_DATA) $</* $(DESTDIR)$(docdir)/html

doxygen-pdf-install: doc/lwsm.pdf
	$(MKDIR_P) -m 755 $(DESTDIR)$(docdir)/pdf
	$(INSTALL_DATA) $< $(DESTDIR)$(docdir)/pdf

all-local: doxygen-html $(DOXYGEN_PDF_TARGET)

install-data-local: doxygen-html-install $(DOXYGEN_PDF_INSTALL_TARGET)

endif

# Make options

.DELETE_ON_ERROR:
