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
        $(AM_CPPFLAGS)                        \
        -DLW_ENABLE_THREADS=1                 \
        $(LWBASE_INCLUDES)                    \
        $(SQLITE_INCLUDES)                    \
        $(CRYPTO_INCLUDES)                    \
        -I$(top_srcdir)/server/include        \
        -I$(top_srcdir)/server/store/include

libsamdb_la_LIBADD =                                    \
        $(UUID_LIBS)                                    \
        $(LWBASE_LIBS)                                  \
        $(SQLITE_LIBS)                                  \
        $(CRYPTO_LIBS)                                  \
        $(top_builddir)/server/store/dsapi/libdsapi.la

libsamdb_la_LDFLAGS =                                                    \
        -module -avoid-version -export-dynamic -export-symbols=samdb.sym \
        $(LWBASE_LDFLAGS)                                                \
        $(SQLITE_LDFLAGS)                                                \
        $(CRYPTO_LDFLAGS) 
