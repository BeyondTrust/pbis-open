AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include
AM_CFLAGS = @AM_CFLAGS@ -Wmissing-prototypes

lib_LTLIBRARIES = liblsagsswrap.la

liblsagsswrap_la_SOURCES = \
       globals.c  \
       gssapi.c   \
       libmain.c

liblsagsswrap_la_CPPFLAGS = \
	$(AM_CPPFLAGS)         \
	-DLW_ENABLE_THREADS=1  \
	-I@top_srcdir@/include \
	@LWBASE_INCLUDES@      \
	@NETLOGON_INCLUDES@

liblsagsswrap_la_LIBADD = \
	@LWBASE_LIBS@   \
	@NETLOGON_LIBS@ \
        @KRB5_LIBS@

