mechanismdir = $(libdir)/AuthMechanisms

mechanism_LTLIBRARIES = ticketcopy.la

AM_CFLAGS=@AM_CFLAGS@ \
	-Wno-unknown-pragmas \
	-pipe -fno-common -fpascal-strings -Wno-four-char-constants \
	-Wall -Werror -mmacosx-version-min=10.5

ticketcopy_la_SOURCES = \
	ticketcopy.c

ticketcopy_la_CFLAGS = $(AM_CFLAGS)

ticketcopy_la_CPPFLAGS = @AM_CPPFLAGS@

ticketcopy_la_LDFLAGS = \
	-module -avoid-version \
	-framework Kerberos

ticketcopy_la_LIBADD = \
	@top_builddir@/client/liblsaclient.la \
	@top_builddir@/interop/lwauthplugin/lib/libauthplugin.la

