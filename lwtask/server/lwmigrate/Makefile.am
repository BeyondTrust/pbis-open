noinst_LTLIBRARIES = liblwmigrate.la

liblwmigrate_la_SOURCES = \
    fileitem.c \
    globals.c  \
    krb5.c     \
    libmain.c  \
    migrate.c

liblwmigrate_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1 \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    @LWNETAPI_INCLUDES@ \
    @KRB5_INCLUDES@
    
liblwmigrate_la_LIBADD = \
    @LWNETAPI_LIBS@ \
    @KRB5_LIBS@
    
liblwmigrate_la_LDFLAGS = -static