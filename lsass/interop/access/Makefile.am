AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = liblsaaccess.la

liblsaaccess_la_SOURCES = \
    access.c

liblsaaccess_la_LIBADD = \
	@top_builddir@/client/liblsaclient.la

liblsaaccess_la_LDFLAGS = \
        -module -avoid-version -export-dynamic \
        @DL_LIBS@
