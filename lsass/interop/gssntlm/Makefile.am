gssconfdir=$(sysconfdir)/gss

gssconf_DATA=mech

gsslibdir=$(libdir)/gss

gsslib_LTLIBRARIES = libgssapi_ntlm.la

libgssapi_ntlm_la_SOURCES =  \
    gssntlm.c

libgssapi_ntlm_la_CFLAGS = \
    $(AM_CFLAGS) \
    $(LWADVAPI_INCLUDES)

libgssapi_ntlm_la_LDFLAGS = \
    $(AM_LDFLAGS) \
    $(LWADVAPI_LDFLAGS) \
    -module -avoid-version -export-dynamic

libgssapi_ntlm_la_LIBADD =  \
    $(top_builddir)/common/liblsacommon.la \
    $(top_builddir)/client/ntlm/liblsaclient_ntlm.la \
    $(KRB5_LIBS)
