AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

provider_LTLIBRARIES = libsamdb.la

libsamdb_la_SOURCES =     \
        samdbadd.c        \
        samdbattrlookup.c \
        samdbbind.c       \
        samdbclose.c      \
        samdbcontext.c    \
        samdbcounter.c    \
        samdbdel.c        \
        samdbdn.c         \
        samdbglobals.c    \
        samdbgroup.c      \
        samdbinit.c       \
        samdbmisc.c       \
        samdbmodify.c     \
        samdbopen.c       \
        samdbschema.c     \
        samdbsearch.c     \
        samdbtrans.c      \
        samdbuser.c       \
        samdbsecurity.c

libsamdb_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
        -DLW_ENABLE_THREADS=1         \
        -I$(top_srcdir)/server/include \
        -I$(top_srcdir)/server/store/include \
        $(SQLITE_INCLUDES) \
        $(CRYPTO_INCLUDES)

libsamdb_la_LIBADD = \
        $(UUID_LIBS) \
        $(top_builddir)/server/store/dsapi/libdsapi.la \
        $(SQLITE_LIBS) \
        $(CRYPTO_LIBS)

libsamdb_la_LDFLAGS = \
        -module -avoid-version -export-dynamic -export-symbols=samdb.sym \
        $(SQLITE_LDFLAGS) \
        $(CRYPTO_LDFLAGS)
