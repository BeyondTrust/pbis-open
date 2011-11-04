ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = $(CPPFLAGS) -Wall -Werror-implicit-function-declaration

AM_CFLAGS += -I$(top_srcdir)/include  \
          -I$(srcdir)/../include \
          -I$(top_srcdir)/libdomainjoin/include   \
          @CENTUTILS_INCLUDE@      \
          $(LWI_OS_FLAGS)

SCRIPTDIR = $(bindir)
AM_CPPFLAGS = -DSCRIPTDIR=\"$(SCRIPTDIR)\" -DLIBDIR=\"$(libdir)\" -DSYSCONFDIR=\"$(sysconfdir)\"

lib_LTLIBRARIES = libdomainjoin-mac.la

libdomainjoin_mac_la_LIBADD = \
	$(top_builddir)/libdomainjoin/libdomainjoin.la

libdomainjoin_mac_la_SOURCES = \
	djitf.c \
	globals.c

libdomainjoin_mac_la_LDFLAGS = \
        -module -avoid-version -export-dynamic

