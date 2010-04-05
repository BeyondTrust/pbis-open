lib_LTLIBRARIES = libgpldap.la

libgpldap_la_SOURCES = \
    gpadirectory.c \
    gpagss.c       \
    gpaquery.c

libgpldap_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/utils

libgpldap_la_LIBADD = \
    @top_builddir@/utils/libgputils.la \
    @top_builddir@/server/auth/libgpauthsvc.la \
    @OPENLDAP_LIBS@ \
    @KRB5_LIBS@

libgpldap_la_LDFLAGS = \
    @OPENLDAP_LDFLAGS@ \
    @KRB5_LDFLAGS@

