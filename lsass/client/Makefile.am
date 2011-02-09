compat_subdirs = \
    api          \
    ad           \
    local

full_subdirs = \
    $(compat_subdirs) \
    ntlm \
    rpc

lib_LTLIBRARIES = liblsaclient.la

liblsaclient_la_SOURCES =

liblsaclient_la_LIBADD = \
    @builddir@/api/liblsaclient_api.la \
    @builddir@/ad/liblsaclient_ad.la \
    @builddir@/local/liblsaclient_local.la

if ENABLE_COMPAT
SUBDIRS=$(compat_subdirs)
else
SUBDIRS=$(full_subdirs)
endif

