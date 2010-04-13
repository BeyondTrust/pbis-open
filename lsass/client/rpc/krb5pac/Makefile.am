rpccli_srcdir = $(top_srcdir)/client/rpc
idl_srcdir = $(top_srcdir)/client/rpc/idl
lsass_srcdir = $(top_srcdir)

noinst_LTLIBRARIES = libkrb5pac.la

libkrb5pac_la_SOURCES = \
	krb5pac_cstub.c \
	krb5pac.c

DCEIDL_PASS_INCLUDES = \
    $(CPPFLAGS)        \
    $(LWBASE_INCLUDES) \
    -I$(lsass_srcdir)/include

libkrb5pac_la_CPPFLAGS =        \
    $(AM_CPPFLAGS)              \
    $(DCERPC_INCLUDES)          \
    -I$(librpc_srcdir)/include

libkrb5pac_la_CFLAGS =    \
    $(AM_CFLAGS)          \
    -DLIBRPC_BUILD

libkrb5pac_la_LDFLAGS = \
    $(AM_LDFLAGS)       \
    $(DCERPC_LDFLAGS)

libkrb5pac_la_LIBADD =  \
    $(DCERPC_LIBS)

BUILT_SOURCES = krb5pac_h.h
include_HEADERS = krb5pac_h.h

CLEANFILES = krb5pac_h.h krb5pac_cstub.c krb5pac_sstub.c

krb5pac_h.h krb5pac_cstub.c: $(idl_srcdir)/krb5pac.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header krb5pac_h.h \
                -I$(idl_srcdir) -I$(srcdir) $(DCEIDL_PASS_INCLUDES) $<
