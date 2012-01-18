noinst_LTLIBRARIES = liblwmigrate.la

liblwmigrate_la_SOURCES = \
    config.c   \
    fileitem.c \
    globals.c  \
    krb5.c     \
    libmain.c  \
    migrate.c  \
    path.c

liblwmigrate_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DLW_ENABLE_THREADS=1 \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    @LWNETAPI_INCLUDES@ \
    @LWREG_INCLUDES@ \
    @KRB5_INCLUDES@
    
liblwmigrate_la_LIBADD = \
    @LWNETAPI_LIBS@ \
    @LWREG_LIBS@ \
    @KRB5_LIBS@
    
liblwmigrate_la_LDFLAGS =
