AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include
AM_CFLAGS = @AM_CFLAGS@ -Wmissing-prototypes

ADDITIONAL_LIBS =
NOINST_DEP_LIBS = \
	@top_builddir@/server/api/liblsaserverapi.la

if ENABLE_STATIC_PROVIDERS
noinst_LTLIBRARIES = liblsass_auth_provider_ad_open.la
if !ENABLE_SMALL_DAEMON
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif
else
provider_LTLIBRARIES = liblsass_auth_provider_ad_open.la
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif

PROVIDER_CPPFLAGS = $(AM_CPPFLAGS)
PROVIDER_LIBADD =
PROVIDER_LDFLAGS =
PROVIDER_CFLAGS = $(AM_CFLAGS)

if !ENABLE_STATIC_PROVIDERS
PROVIDER_CFLAGS += -shared
PROVIDER_LDFLAGS += -shared -module -avoid-version
endif

liblsass_auth_provider_ad_open_la_SOURCES = \
       adcache.c                 \
       adcfg.c                   \
       adldap.c                  \
       ad_marshal_user.c         \
       ad_marshal_group.c        \
       ad_marshal_nss_artefact.c \
       adnetapi.c                \
       state_store.c             \
       cellldap.c                \
       defldap.c                 \
       enumstate.c               \
       globals.c                 \
       machinepwd.c              \
       offline.c                 \
       offline-helper.c          \
       online.c                  \
       provider-main.c           \
       providerstate.c           \
       lsadm.c                   \
       lsadm_p.c                 \
       lsadmengine.c             \
       lsadmwrap.c               \
       media-sense.c             \
       batch.c                   \
       lsaum.c                   \
       lsaum_p.c                 \
       lsaumproc.c               \
       batch_build.c             \
       batch_gather.c            \
       batch_marshal.c           \
       batch_enum.c              \
       memcache.c                \
       specialdomain.c           \
       unprov.c

if !ENABLE_MINIMAL
PROVIDER_CPPFLAGS += -DAD_CACHE_ENABLE_SQLITE @SQLITE_INCLUDES@
PROVIDER_LIBADD += @SQLITE_LIBS@
PROVIDER_LDFLAGS += @SQLITE_LDFLAGS@
liblsass_auth_provider_ad_open_la_SOURCES += \
       sqlcache.c                \
       lsasqlite.c
endif

liblsass_auth_provider_ad_open_la_CPPFLAGS = \
	$(AM_CPPFLAGS)                \
    -DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
        $(PROVIDER_CPPFLAGS)          \
	@PSTORE_INCLUDES@             \
	@NETAPI_INCLUDES@             \
	@NETLOGON_INCLUDES@

liblsass_auth_provider_ad_open_la_CFLAGS = $(AM_CFLAGS) $(PROVIDER_CFLAGS)

# NOTE: Need to remove LSA client dependency.
liblsass_auth_provider_ad_open_la_LIBADD = \
	@top_builddir@/common/liblsacommon.la \
	@top_builddir@/join/liblsajoin.la \
	$(ADDITIONAL_LIBS) \
	@PSTORE_LIBS@ \
	@NETAPI_LIBS@ \
	@NETLOGON_LIBS@ \
	@PROC_LIBS@ \
	@KVM_LIBS@  \
	@UUID_LIBS@ \
	@CRYPTO_LIBS@ \
        $(PROVIDER_LIBADD)

liblsass_auth_provider_ad_open_la_LDFLAGS = \
	$(PROVIDER_LDFLAGS) \
	@PSTORE_LDFLAGS@ \
	@NETAPI_LDFLAGS@ \
	@NETLOGON_LDFLAGS@ \
	@CF_FRAMEWORK_LDFLAGS@ \
	@SC_FRAMEWORK_LDFLAGS@ \
	@CRYPTO_LDFLAGS@ 
