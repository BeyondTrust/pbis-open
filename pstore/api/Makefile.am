AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = liblwpsapi.la

liblwpsapi_la_SOURCES =  \
	libmain.c        \
	lwps-provider.c

liblwpsapi_la_LIBADD = @top_builddir@/utils/liblwps-utils.la @DL_LIBS@ @LWREG_LDFLAGS@ @PTHREAD_LIBS@

liblwpsapi_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        @LWREG_INCLUDES@

if ENABLE_STATIC_PROVIDERS
if ENABLE_SQLDB
liblwpsapi_la_LIBADD += $(top_builddir)/providers/sqldb/liblwps-sqldb.la
endif
if ENABLE_FILEDB
liblwpsapi_la_LIBADD += $(top_builddir)/providers/filedb/liblwps-filedb.la
endif
endif

if ENABLE_REGDB
liblwpsapi_la_LIBADD += @LWREG_LIBS@
endif


noinst_HEADERS = \
	lwps-provider_p.h
