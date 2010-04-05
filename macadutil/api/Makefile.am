lib_LTLIBRARIES = libmacadutil.la

libmacadutil_la_SOURCES = \
    adinfo.c         \
    aducopy.c        \
    adukrb5.c        \
    api.c            \
    credcontext.c    \
    directory.c      \
    filexfer.c       \
    globals.c        \
    gpcache.c        \
    gss.c            \
    mcxutil.c        \
    policyutils.c    \
    xfer.c
       
libmacadutil_la_CPPFLAGS = \
	@NETLOGON_INCLUDES@ \
	@GROUPPOLICY_INCLUDES@ \
	@LSASS_INCLUDES@ \
        @LWIO_INCLUDES@

libmacadutil_la_LIBADD = \
	@top_builddir@/lwutils/liblwutils.la \
	@LSASS_LIBS@                         \
	@GROUPPOLICY_LIBS@                   \
	@NETLOGON_LIBS@                      \
        @LWIO_LIBS@                          \
        @OPENLDAP_LIBS@                      \
        @KRB5_LIBS@                          \
        @LWREG_LIBS@                          \
        @PTHREAD_LIBS@

libmacadutil_la_LDFLAGS = \
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
    mcxutil.h        \
    structs.h        \
    aducopy.h        \
    credcontext.h

