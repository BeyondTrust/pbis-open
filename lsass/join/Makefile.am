SUBDIRS =   \
    keytab

AM_CPPFLAGS =                \
    @AM_CPPFLAGS@            \
    -I$(top_srcdir)/include

lib_LTLIBRARIES = liblsajoin.la

liblsajoin_la_SOURCES = \
    error.c          \
    globals.c        \
    join.c           \
    ldaputil.c       \
    leave.c          \
    libmain.c        \
    lsakrb5smb.c     \
    lsaldap.c        \
    query.c


liblsajoin_la_CPPFLAGS =             \
    $(AM_CPPFLAGS)                   \
    -DLW_ENABLE_THREADS=1            \
    -I$(top_srcdir)/server/include   \
    -I$(top_srcdir)/join/keytab      \
    $(NETLOGON_INCLUDES)             \
    $(PSTORE_INCLUDES)               \
    $(KRB5_INCLUDES)                 \
    $(LWIO_INCLUDES)


liblsajoin_la_LIBADD =                         \
    $(top_builddir)/common/liblsacommon.la     \
    $(top_builddir)/client/rpc/liblsarpc.la    \
    $(top_builddir)/join/keytab/libktkrb5.la   \
    $(NETLOGON_LIBS)                           \
    $(PSTORE_LIBS)                             \
    $(top_builddir)/logging_r/liblsalog_r.la   \
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
