AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include
AM_CFLAGS = @AM_CFLAGS@ -Wmissing-prototypes

if ENABLE_STATIC_PROVIDERS
noinst_LTLIBRARIES = liblsass_auth_provider_local.la
PROVIDER_CFLAGS =
PROVIDER_LDFLAGS =
else
provider_LTLIBRARIES = liblsass_auth_provider_local.la
PROVIDER_CFLAGS = -shared
PROVIDER_LDFLAGS = -shared -module -avoid-version
endif

liblsass_auth_provider_local_la_SOURCES = \
    lpaccess.c    \
    lpauthex.c    \
    lpcfg.c       \
    lpdomain.c    \
    lpenumstate.c \
    lpevent.c     \
    lpglobals.c   \
    lpgroup.c     \
    lpmain.c      \
    lpmarshal.c   \
    lpmisc.c      \
    lpobject.c    \
    lpuser.c      \
    lpsecurity.c  \
    lpprivilege.c

liblsass_auth_provider_local_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DLW_ENABLE_THREADS=1              \
    -I@top_srcdir@/server/include

liblsass_auth_provider_local_la_CFLAGS = $(AM_CFLAGS) $(PROVIDER_CFLAGS)

#netapi is needed for NTLMv1EncryptChallenge
liblsass_auth_provider_local_la_LIBADD = \
    @top_builddir@/common/liblsacommon.la \
    @top_builddir@/server/api/liblsaserverapi.la \
    @top_builddir@/server/store/dsapi/libdsapi.la

liblsass_auth_provider_local_la_LDFLAGS = \
	$(PROVIDER_LDFLAGS) \
	-export-symbols=lp.sym
