rpccli_builddir = $(top_builddir)/client/rpc

SUBDIRS =    \
    lsa      \
    samr     \
    netlogon \
    dssetup  \
    krb5pac  \
    wkssvc

lib_LTLIBRARIES = liblsarpc.la

liblsarpc_la_SOURCES =

liblsarpc_la_LIBADD =                           \
    @top_builddir@/common/liblsacommon.la       \
    $(rpccli_builddir)/lsa/liblsa.la            \
    $(rpccli_builddir)/samr/libsamr.la          \
    $(rpccli_builddir)/netlogon/libnetlogon.la  \
    $(rpccli_builddir)/dssetup/libdssetup.la    \
    $(rpccli_builddir)/krb5pac/libkrb5pac.la    \
    $(rpccli_builddir)/wkssvc/libwkssvc.la
