AM_CPPFLAGS =                \
    @AM_CPPFLAGS@            \
    -I$(top_srcdir)/include

noinst_LTLIBRARIES = liblsajoin.la

liblsajoin_la_SOURCES = \
    join.c           \
    ldaputil.c       \
    leave.c          \
    lsakrb5smb.c     \
    lsaldap.c        \
    keytab.c         \
    ktldap.c


liblsajoin_la_CPPFLAGS =             \
    $(AM_CPPFLAGS)                   \
    -DLW_ENABLE_THREADS=1            \
    -I$(srcdir)/include              \
    -I$(top_srcdir)/server/include   \
    $(NETLOGON_INCLUDES)             \
    $(PSTORE_INCLUDES)               \
    $(KRB5_INCLUDES)                 \
    $(LWIO_INCLUDES)


liblsajoin_la_LIBADD =                         \
    $(top_builddir)/common/liblsacommon.la     \
    $(top_builddir)/client/rpc/liblsarpc.la    \
    $(NETLOGON_LIBS)                           \
    $(PSTORE_LIBS)                             \
    $(KRB5_LIBS)                               \
    $(LWIO_LIBS)                               \
    $(LWADVAPI_LIBS)                           \
    -llber

liblsajoin_la_LDFLAGS =    \
    $(NETLOGON_LDFLAGS)    \
    $(PSTORE_LDFLAGS)      \
    $(OPENLDAP_LDFLAGS)    \
    $(KRB5_LDFLAGS)        \
    $(LWIO_LDFLAGS)
