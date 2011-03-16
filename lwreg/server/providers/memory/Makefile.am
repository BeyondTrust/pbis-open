noinst_LTLIBRARIES = libregserver_provider_memory.la

PROVIDER_CPPFLAGS = @MEMORY_INCLUDES@
PROVIDER_LIBS = @MEMORY_LIBS@
PROVIDER_LDFLAGS = @MEMORY_LDFLAGS@

libregserver_provider_memory_la_SOURCES = \
    globals.c \
    memacl.c \
    memapi.c \
    memdb.c \
    memstore.c \
    memschema.c


libregserver_provider_memory_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1         \
    $(AM_CPPFLAGS)                \
    -I@top_srcdir@/server/include \
    @LWBASE_INCLUDES@             \
    $(PROVIDER_CPPFLAGS)

libregserver_provider_memory_la_LIBADD = \
    $(PROVIDER_LIBS) \
    @PTHREAD_LIBS@ \
    @UUID_LIBS@ \
    @LWBASE_LIBS@

libregserver_provider_memory_la_LDFLAGS = \
    @LWBASE_LDFLAGS@ \
    $(PROVIDER_LDFLAGS)

