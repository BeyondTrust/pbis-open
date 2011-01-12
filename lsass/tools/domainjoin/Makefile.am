noinst_PROGRAMS = lsadomainjoin-test

lsadomainjoin_test_SOURCES = \
    join.c

lsadomainjoin_test_LDADD = \
    $(AM_LDFLAGS) \
    @top_builddir@/client/liblsaclient.la \
    -llwnetclientapi

lsadomainjoin_test_CPPFLAGS = \
    $(AM_CPPFLAGS)
