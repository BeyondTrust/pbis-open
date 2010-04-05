lib_LTLIBRARIES = liblwadutilapi.la

liblwadutilapi_la_SOURCES = \
    api.c            \
    credcontext.c    \
    globals.c        \
    adinfo.c         \
    directory.c      \
    filexfer.c       \
    gss.c            \
    adukrb5.c        \
    policyutils.c    \
    xfer.c           \
    gpcache.c        \
    adutil.c         \
    aducopy.c

liblwadutilapi_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	@NETLOGON_INCLUDES@ \
	@GROUPPOLICY_INCLUDES@ \
	@LSASS_INCLUDES@ \
        @LWIO_INCLUDES@

liblwadutilapi_la_LIBADD = \
	@top_builddir@/lwutils/liblwutils.la \
	@LSASS_LIBS@                         \
	@GROUPPOLICY_LIBS@                   \
	@NETLOGON_LIBS@                      \
        @LWIO_LIBS@                          \
    @OPENLDAP_LIBS@                      \
    @KRB5_LIBS@                          \
    @PTHREAD_LIBS@

liblwadutilapi_la_LDFLAGS = \
	-module -avoid-version -export-dynamic \
	@NETLOGON_LDFLAGS@                     \
	@LSASS_LDFLAGS@                        \
	@GROUPPOLICY_LDFLAGS@                  \
        @LWIO_LDFLAGS@                         \
    @OPENLDAP_LDFLAGS@                     \
    @KRB5_LDFLAGS@

noinst_HEADERS = \
    api.h            \
    defs.h           \
    externs.h        \
    adinfo.h         \
    directory.h      \
    filexfer.h       \
    gss.h            \
    adukrb5.h        \
    policyutils.h    \
    xfer.h           \
    gpcache.h        \
    includes.h       \
    adutil.h        \
    structs.h       \
    aducopy.h       \
    credcontext.h
    

