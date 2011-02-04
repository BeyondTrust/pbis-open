SUBDIRS = \
    api \
    ad \
    local \
    ntlm

noinst_LTLIBRARIES = liblsaipc.la

liblsaipc_la_SOURCES =

liblsaipc_la_LIBADD = \
    @builddir@/api/liblsaipc_api.la \
    @builddir@/ad/liblsaipc_ad.la \
    @builddir@/local/liblsaipc_local.la \
    @builddir@/ntlm/liblsaipc_ntlm.la
