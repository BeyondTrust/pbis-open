noinst_LTLIBRARIES = libregserver_provider_sqlite.la

PROVIDER_CPPFLAGS = @SQLITE_INCLUDES@
PROVIDER_LIBS = @SQLITE_LIBS@
PROVIDER_LDFLAGS = @SQLITE_LDFLAGS@

libregserver_provider_sqlite_la_SOURCES = \
    sqliteapi.c    \
    sqliteacl.c   \
    sqliteapi_p.c    \
    sqldb.c     \
    sqldb_p.c     \
    sqlitecache.c     \
    regsqlite.c    \
    globals.c           

libregserver_provider_sqlite_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1         \
    $(AM_CPPFLAGS)                \
    -I@top_srcdir@/server/include \
    @LWBASE_INCLUDES@ \
    $(PROVIDER_CPPFLAGS)          

libregserver_provider_sqlite_la_LIBADD = \
    $(PROVIDER_LIBS) \
    @PTHREAD_LIBS@    \
    @UUID_LIBS@ \
    @LWBASE_LIBS@
    
libregserver_provider_sqlite_la_LDFLAGS = \
    @LWBASE_LDFLAGS@ \
    $(PROVIDER_LDFLAGS)
