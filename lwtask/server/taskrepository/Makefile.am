noinst_LTLIBRARIES = libtaskrepository.la

libtaskrepository_la_SOURCES = \
    db.c      \
    globals.c \
    libmain.c

libtaskrepository_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DLW_ENABLE_THREADS=1         \
    -I@top_srcdir@/include        \
    -I@top_srcdir@/server/include \
    @SQLITE_INCLUDES@
    
libtaskrepository_la_LIBADD = \
    @SQLITE_LIBS@
    
libtaskrepository_la_LDFLAGS =
