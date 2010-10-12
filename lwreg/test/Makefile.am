noinst_PROGRAMS = test_regio test_reglex test_regparse test_regiconv test_regdump test_tomulti test_ptlwregd test_regvalueattrs

#test_regio
test_regio_SOURCES = \
    test_regio.c

test_regio_LDADD =                           \
    @top_builddir@/parse/libregparse.la

test_regio_LDFLAGS = -static

test_regio_CPPFLAGS = \
    $(AM_CPPFLAGS)      \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@ \
    -I@top_srcdir@ \
    -I@top_srcdir@/server/include

#test_reglex
test_reglex_SOURCES = \
    test_reglex.c

test_reglex_LDADD =                           \
    @top_builddir@/parse/libregparse.la

test_reglex_LDFLAGS = -static
test_reglex_CPPFLAGS = \
    $(AM_CPPFLAGS)       \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@ \
    -I@top_srcdir@ \
    -I@top_srcdir@/server/include

test_regparse_SOURCES = \
    test_regparse.c


#test regparse
test_regparse_LDADD =                           \
    @top_builddir@/shellutil/librsutils.la \
    @top_builddir@/client/libregclient.la \
    @top_builddir@/parse/libregparse.la

test_regparse_LDFLAGS = -static
test_regparse_CPPFLAGS = \
    $(AM_CPPFLAGS)       \
    -I@top_srcdir@/server/include \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@ \
    -I@top_srcdir@ \
    -I@top_srcdir@/server/include

#test regdump
test_regdump_SOURCES = \
    test_regdump.c

test_regdump_LDADD =                           \
    @top_builddir@/shellutil/librsutils.la \
    @top_builddir@/client/libregclient.la \
    @top_builddir@/parse/libregparse.la

test_regdump_LDFLAGS = -static
test_regdump_CPPFLAGS = \
    $(AM_CPPFLAGS)       \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@ \
    -I@top_srcdir@ \
    -I@top_srcdir@/server/include

#test regiconv
test_regiconv_SOURCES = \
    test_regiconv.c

test_regiconv_LDADD =                           \
    @top_builddir@/parse/libregparse.la

test_regiconv_LDFLAGS = -static
test_regiconv_CPPFLAGS = \
    $(AM_CPPFLAGS)       \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@ \
    -I@top_srcdir@ \
    -I@top_srcdir@/server/include

#test tomulti
test_tomulti_SOURCES = \
    test_tomulti.c

test_tomulti_LDADD =                           \
    @top_builddir@/shellutil/librsutils.la \
    @top_builddir@/client/libregclient.la \
    @top_builddir@/parse/libregparse.la

test_tomulti_LDFLAGS = -static
test_tomulti_CPPFLAGS = \
    $(AM_CPPFLAGS)       \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@ \
    -I@top_srcdir@ \
    -I@top_srcdir@/server/include

#test_ptlwregd
test_ptlwregd_SOURCES = \
    test_ptlwregd.c

test_ptlwregd_LDADD =                           \
    @top_builddir@/shellutil/librsutils.la \
    @top_builddir@/client/libregclient.la \
    @PTHREAD_LIBS@


test_ptlwregd_LDFLAGS = -static

test_ptlwregd_CPPFLAGS = \
    $(AM_CPPFLAGS)   \
    -I@top_srcdir@/shellutil \
    @LWBASE_INCLUDES@
    
    
#test_regvalueattrs
test_regvalueattrs_SOURCES = \
    test_regvalueattrs.c

test_regvalueattrs_LDADD =                           \
    @top_builddir@/shellutil/librsutils.la \
    @top_builddir@/client/libregclient.la \
    @PTHREAD_LIBS@


test_regvalueattrs_LDFLAGS = -static

test_regvalueattrs_CPPFLAGS = \
    $(AM_CPPFLAGS)   \
    -I@top_srcdir@/shellutil \
    @LWBASE_INCLUDES@
