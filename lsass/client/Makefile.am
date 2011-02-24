compat_subdirs = \
    api          \
    ad           \
    privilege

full_subdirs = \
    $(compat_subdirs) \
    ntlm \
    rpc

lib_LTLIBRARIES = liblsaclient.la liblsaclientthr.la

liblsaclient_la_SOURCES =

liblsaclient_la_LIBADD = \
    @builddir@/api/liblsaclient_api.la \
    @builddir@/ad/liblsaclient_ad.la \
    @builddir@/privilege/liblsaclient_privilege.la

liblsaclientthr_la_SOURCES =

liblsaclientthr_la_LIBADD = \
    @builddir@/api/liblsaclient_api_thr.la \
    liblsaclient.la

if ENABLE_COMPAT
SUBDIRS=$(compat_subdirs)
else
SUBDIRS=$(full_subdirs)
endif

