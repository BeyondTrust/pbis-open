AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_PROGRAMS = test_headers

test_headers_CPPFLAGS=\
    $(AM_CPPFLAGS) \
    @NETLOGON_INCLUDES@

test_headers_SOURCES = \
    before.c \
    after.c \
    main.c

test_headers_LDFLAGS = -static
test_headers_LDADD = @top_builddir@/client/liblsaclient.la
