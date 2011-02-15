SUBDIRS = \
    api \
    ad \
    privilege \
    ntlm

noinst_LTLIBRARIES = liblsaipc.la

liblsaipc_la_SOURCES =

liblsaipc_la_LIBADD = \
    @builddir@/api/liblsaipc_api.la \
    @builddir@/ad/liblsaipc_ad.la \
    @builddir@/privilege/liblsaipc_privilege.la \
    @builddir@/ntlm/liblsaipc_ntlm.la
